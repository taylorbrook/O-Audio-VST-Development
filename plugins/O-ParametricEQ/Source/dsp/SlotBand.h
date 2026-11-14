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

    O-ParametricEQ - band runtime, two slots (Stage 2, Phase 2.4)

    One of the 24 bands as the processor drives it since 2.4: two unmodified
    eq::Band slots (Band.h, the one-slot runtime of 2.2 / 2.3), each with a
    placement and an off flag, and what is added around them: the structural
    crossfade, the latched request, the neutral sleep and the wake
    (PLAN-2.4 P3, P4; ARCHITECTURE "Structural changes: two-slot crossfade").

    Slots. `cur` is the slot that runs when no fade is in flight; during a
    fade it is the OUTGOING one and 1 - cur the incoming one. "Off" is a
    flag: an off slot is never designed at unity, never ticked and never
    processed, and where a fade needs its output that output is the dry band
    input. An off slot's state is always at rest.

    Two inputs are kept: `in`, what the incoming slot (or the current one)
    realises, and `desired`, the latest request. A request is LATCHED while a
    fade is in flight and the two differ; that is derived from the two on
    every call and stored nowhere.

    A target is "off" when it is inactive, or an active gain type (Bell, the
    shelves, Tilt) at exactly 0 dB; the second case is "asleep".

    The request table. request (input) stores `desired`, then:

      fade in flight
        continuous against `in`, input active,     incoming slot setTarget;
        band not asleep, incoming slot not off     in = input
        anything else                              latched: waits for the
                                                   fade's end

      no fade in flight: settle (input), by classifyBandChange (in, input)
        identical                                  nothing
        continuous, input inactive                 in = input
        continuous, asleep, gain still exactly 0   in = input
        continuous, asleep, gain no longer 0       WAKE: the idle slot is
                                                   configured from the input
                                                   with the gain at 0 dB,
                                                   then setTarget (input);
                                                   the fade from the off
                                                   slot starts; in = input
        continuous otherwise                       current slot setTarget;
                                                   in = input
        structural, target off, current slot off   in = input; asleep as the
                                                   target says; no fade
        structural otherwise                       the idle slot is left off
                                                   (target off) or configured
                                                   from the full target; the
                                                   fade starts; in = input;
                                                   asleep as the target says

    The end of a fade: the outgoing slot is retired, `cur` flips, and then,
    if `desired` differs from `in`, settle (desired) runs at once. The
    comparison is with what the band realises at that moment, so a request
    for B followed by a request back to the fade's own target starts
    nothing. A fade that settle starts there begins on the very next sample.

    A structural target is always designed in full (type, slope, placement,
    frequency, gain, Q) and fades in settled: no control ramp runs on an
    incoming slot unless a continuous change reaches it during the fade, or
    it is a wake.

    The fade. In pieces of at most 64 samples that end at the fade's end,
    each slot that is not off runs through the Router on its own stack copy
    of the band input, with its own placement; the output is

      out = a + w (b - a)     a = outgoing copy, b = incoming copy,
                              w = table.weight (pos + i)

    on every lane either slot touches. A lane neither slot touches is not
    read and not written (its pointer may be null), which is what the mix
    would give anyway: a and b would be the same bits. The incoming slot
    starts from rest at the fade's first sample (no pre-roll in v1; a
    pre-roll in the table makes it run at weight 0 first).

    The sleep test, at a control tick, only when no fade is in flight:

      the current slot is not off
      `in` is active and a gain type
      in.settings.gainDbEff is exactly 0
      the current slot is not moving (its ramp has arrived)
      the current slot's dynamic offset is exactly 0

    for sleepTicks consecutive ticks (sleepLengthTicks, Fade.h); then the
    band is asleep and the fade to the off slot starts. Any other tick zeroes
    the count. The test is on the target and on the ramp having arrived,
    never on the ramped value (a ramp that passes through 0 dB does not
    satisfy it), and on the gain alone: the skewed frequency and Q defaults
    are inexact. A 0 dB gain design is the identity to rounding and not bit
    for bit, which is why the band leaves the list.

    The three ways a slot's filter state is cleared, and no other:

      configure    snapTo (settings) + reset: a slot about to fade in, or to
                   run settled from rest (startSettled); placement set, off
                   cleared
      retire       reset + snapToTarget, off set: the outgoing slot at a
                   fade's end, every slot in goIdle and at the top of
                   startSettled
      clearState   reset + snapToTarget on both slots, off flags and the
                   fade untouched: the non-finite path of the processor

    (prepare also resets both slots; it is not a path of a running band.)

    One place each (PLAN-2.4 R7): `fading` is set in startFade and cleared in
    stopFade; `asleep` is written in markAsleep; the latch is derived.

    The dynamic offset stays stored in eq::Band alone. This class reads the
    current slot's value for the sleep test and stores none of its own;
    wiring a non-zero offset to both slots is v1.x.

    Call order, per host block: request (or startSettled) at the block
    start; then per segment, controlTick when the segment starts on a grid
    tick, then process. Both inputs carry the EFFECTIVE placement
    (effectivePlacement, BandParams.h) and are in canonical form. The
    designer is a template parameter as in Band.h: anything with
    design (const BandSettings&, BandDesign&).

    Callers: the processor, the designer harness.

    Standard C++17 only; allocation-free; every function is noexcept.

  ==============================================================================
*/

#pragma once

#include "Band.h"
#include "BandParams.h"
#include "EqTypes.h"
#include "Fade.h"
#include "Router.h"

#include <cassert>

namespace eq
{

class SlotBand
{
public:
    /** The largest piece of a fade processed on one pair of stack copies. */
    static constexpr int fadePieceSize = 64;

    //==========================================================================
    /** Once per prepare: K and the ramp length for both slots, their state
        at rest, both off, no fade, `in` and `desired` inactive, awake, the
        counters at 0. */
    void prepare (int gridSize, int rampTicks) noexcept
    {
        for (int i = 0; i < 2; ++i)
        {
            slot[i].setControlGrid (gridSize, rampTicks);
            slot[i].reset();
            place[i] = Placement::stereo;
            off[i] = true;
        }

        cur = 0;
        stopFade();
        markAsleep (false);
        zeroTicks = 0;

        in = inactiveInput();
        desired = in;

        fadesStarted = 0;
        latchedStarted = 0;
        sleepCount = 0;
        wakeCount = 0;
    }

    /** The band from rest on `input`, settled: no fade and no ramp. A fade
        in flight is dropped and both slots are retired first. Then:
        inactive -> both off; an active gain type at exactly 0 dB -> both
        off, asleep; otherwise the current slot is configured from the
        input. in = desired = input. For the first block after a prepare and
        for un-bypass, where the bypass fade covers the start. */
    template <typename DesignerType>
    void startSettled (const BandInput& input, DesignerType& designer) noexcept
    {
        stopFade();
        retire (0, designer);
        retire (1, designer);

        const bool sleeps = input.active && gainIsZero (input);

        if (input.active && ! sleeps)
            configure (cur, input.settings, input.placement, designer);

        markAsleep (sleeps);
        zeroTicks = 0;
        in = input;
        desired = input;
    }

    /** At a host block start, with the band's canonical input. See the
        request table in the file comment. */
    template <typename DesignerType>
    void request (const BandInput& input, DesignerType& designer) noexcept
    {
        desired = input;

        if (fading)
        {
            const int to = 1 - cur;

            // A continuous change reaches the incoming slot at once; anything else waits for the fade's end.
            if (classifyBandChange (in, input) == BandChange::continuous
                && input.active
                && ! asleep
                && ! off[to])
            {
                slot[to].setTarget (input.settings);
                in = input;
            }

            return;
        }

        settle (input, designer);
    }

    /** Once per grid tick, before the samples of the interval: the control
        tick of every slot that is not off (both during a fade), then the
        sleep test of the file comment. sleepTicks = sleepLengthTicks (fs, K);
        a value below 1 is read as 1. */
    template <typename DesignerType>
    void controlTick (DesignerType& designer, int sleepTicks) noexcept
    {
        if (! off[0])
            slot[0].controlTick (designer);

        if (! off[1])
            slot[1].controlTick (designer);

        const bool atZero = ! fading
                         && ! off[cur]
                         && in.active
                         && gainIsZero (in)
                         && ! slot[cur].isMoving()
                         && sameValue (slot[cur].dynOffsetDb, 0.0);

        if (! atZero)
        {
            zeroTicks = 0;
            return;
        }

        ++zeroTicks;

        if (zeroTicks >= sleepTicks)
        {
            // The idle slot is off already (it was retired); the band fades to it.
            ++sleepCount;
            markAsleep (true);
            off[1 - cur] = true;
            startFade();
        }
    }

    /** Runs the band over `numSamples` samples of the lanes, in place.
        `left` is lane 0, `right` lane 1; a lane for which touchesLane is
        false is neither read nor written and its pointer may be null. No
        fade: the current slot through the Router with its placement, or
        nothing when it is off. Fade: see the file comment. When the fade
        ends inside the call the rest of the samples continue in the same
        call, on a latched request's fade if one starts. */
    template <typename DesignerType>
    void process (double* left, double* right, int numSamples, bool stereo,
                  const FadeTable& table, DesignerType& designer) noexcept
    {
        double* io[2] = { left, right };
        int remaining = numSamples;

        while (remaining > 0)
        {
            if (! fading)
            {
                if (! off[cur])
                    Router::process (slot[cur], place[cur], io[0], io[1], remaining, stereo);

                return;
            }

            const int length = table.getLength();

            if (pos >= length)
            {
                endFade (designer);
                continue;
            }

            int count = length - pos;

            if (count > remaining)
                count = remaining;

            if (count > fadePieceSize)
                count = fadePieceSize;

            const int from = cur;
            const int to = 1 - cur;

            const bool used[2] = { slotTouches (from, 0, stereo) || slotTouches (to, 0, stereo),
                                   slotTouches (from, 1, stereo) || slotTouches (to, 1, stereo) };

            // Two copies of the band input: an off slot's copy stays the dry input.
            double outgoing[2][fadePieceSize];
            double incoming[2][fadePieceSize];

            for (int l = 0; l < 2; ++l)
            {
                if (! used[l])
                    continue;

                assert (io[l] != nullptr);

                for (int i = 0; i < count; ++i)
                {
                    const double x = io[l][i];
                    outgoing[l][i] = x;
                    incoming[l][i] = x;
                }
            }

            if (! off[from])
                Router::process (slot[from], place[from], outgoing[0], outgoing[1], count, stereo);

            if (! off[to])
                Router::process (slot[to], place[to], incoming[0], incoming[1], count, stereo);

            for (int l = 0; l < 2; ++l)
            {
                if (! used[l])
                    continue;

                for (int i = 0; i < count; ++i)
                {
                    const double w = table.weight (pos + i);
                    const double a = outgoing[l][i];
                    const double b = incoming[l][i];
                    io[l][i] = a + w * (b - a);
                }
            }

            pos += count;
            remaining -= count;

            for (int l = 0; l < 2; ++l)
                if (io[l] != nullptr)
                    io[l] += count;

            if (pos >= length)
                endFade (designer);
        }
    }

    /** The engine goes idle (fully bypassed). A fade in flight ends on its
        target: the outgoing slot retired, `cur` flipped, and the latched
        request, if any, starts nothing. Then the current slot is retired as
        well. `in` becomes inactive, the band is awake, the sleep count 0.
        `desired` is kept: startSettled (getDesired()) brings the band back. */
    template <typename DesignerType>
    void goIdle (DesignerType& designer) noexcept
    {
        if (fading)
            finishFade (designer);

        if (! off[cur])
            retire (cur, designer);

        in.active = false;
        markAsleep (false);
        zeroTicks = 0;
    }

    /** Clears the filter state of both slots and snaps their ramps
        (reset + snapToTarget). Off flags, the fade and its position are
        untouched: a fade in flight carries on. For non-finite input. */
    template <typename DesignerType>
    void clearState (DesignerType& designer) noexcept
    {
        for (int i = 0; i < 2; ++i)
        {
            slot[i].reset();
            slot[i].snapToTarget (designer);
        }
    }

    //==========================================================================
    /** On the active list: a fade is in flight, or the current slot is not
        off. A band asleep, disabled or bypassed with no fade in flight is
        not running. */
    bool isRunning() const noexcept                             { return fading || ! off[cur]; }

    bool isFading() const noexcept                              { return fading; }
    bool isAsleep() const noexcept                              { return asleep; }

    /** True while a fade is in flight and the latest request differs from
        what the band realises. Derived on every call. */
    bool hasLatchedRequest() const noexcept
    {
        return fading && classifyBandChange (in, desired) != BandChange::identical;
    }

    /** True when the band may read or write lane `lane` (0 or 1) before the
        next request: the placement of every slot that is not off, and,
        where a request is latched, the placement of `desired` when it is
        active and not a gain type at exactly 0 dB (PLAN-2.4 P7). */
    bool touchesLane (int lane, bool stereo) const noexcept
    {
        if (slotTouches (0, lane, stereo) || slotTouches (1, lane, stereo))
            return true;

        return hasLatchedRequest()
            && desired.active
            && ! gainIsZero (desired)
            && Router::touchesLane (desired.placement, lane, stereo);
    }

    //==========================================================================
    // For the harness and the display: read only
    //==========================================================================

    /** Slot `index` (0 or 1; any other value reads slot 0). */
    const Band& getSlot (int index) const noexcept              { return slot[(index == 1) ? 1 : 0]; }

    Placement getSlotPlacement (int index) const noexcept       { return place[(index == 1) ? 1 : 0]; }
    bool isSlotOff (int index) const noexcept                   { return off[(index == 1) ? 1 : 0]; }

    /** The slot that runs when no fade is in flight; during a fade, the
        outgoing one. */
    int getCurrentSlot() const noexcept                         { return cur; }

    /** The fade sample the next process call mixes first; 0 with no fade. */
    int getFadePosition() const noexcept                        { return pos; }

    /** What the incoming slot (or the current one) realises. */
    const BandInput& getRealised() const noexcept               { return in; }

    /** The latest request. */
    const BandInput& getDesired() const noexcept                { return desired; }

    /** Consecutive control ticks the sleep test has held. */
    int getZeroTicks() const noexcept                           { return zeroTicks; }

    /** Since the last prepare: fades started (structural, sleep and wake
        fades alike); fade ends that found `desired` different from `in` and
        settled on it; sleeps; wakes. */
    unsigned long getFadesStarted() const noexcept              { return fadesStarted; }
    unsigned long getLatchedStarted() const noexcept            { return latchedStarted; }
    unsigned long getSleepCount() const noexcept                { return sleepCount; }
    unsigned long getWakeCount() const noexcept                 { return wakeCount; }

private:
    static bool gainIsZero (const BandInput& input) noexcept
    {
        return isGainType (input.settings.type) && sameValue (input.settings.gainDbEff, 0.0);
    }

    static BandInput inactiveInput() noexcept
    {
        BandInput input;
        input.settings = BandSettings { Type::bell, 1000.0, 0.0, 1.0, 0 };
        input.placement = Placement::stereo;
        input.active = false;
        return input;
    }

    bool slotTouches (int index, int lane, bool stereo) const noexcept
    {
        return ! off[index] && Router::touchesLane (place[index], lane, stereo);
    }

    //==========================================================================
    // The only writers of `fading` and `asleep`
    //==========================================================================

    void startFade() noexcept
    {
        fading = true;
        pos = 0;
        zeroTicks = 0;
        ++fadesStarted;
    }

    void stopFade() noexcept
    {
        fading = false;
        pos = 0;
    }

    void markAsleep (bool value) noexcept
    {
        asleep = value;
    }

    //==========================================================================
    // Slot state: configure and retire (clearState is public)
    //==========================================================================

    /** The slot designed from `settings`, settled, state at rest. */
    template <typename DesignerType>
    void configure (int index, const BandSettings& settings, Placement placement, DesignerType& designer) noexcept
    {
        slot[index].snapTo (settings, designer);
        slot[index].reset();
        place[index] = placement;
        off[index] = false;
    }

    /** The slot at rest, its ramp snapped, and off. */
    template <typename DesignerType>
    void retire (int index, DesignerType& designer) noexcept
    {
        slot[index].reset();
        slot[index].snapToTarget (designer);
        off[index] = true;
    }

    //==========================================================================
    /** No fade is in flight. Brings the band to `next`. */
    template <typename DesignerType>
    void settle (const BandInput& next, DesignerType& designer) noexcept
    {
        const BandChange change = classifyBandChange (in, next);

        if (change == BandChange::identical)
            return;

        const bool nextSleeps = next.active && gainIsZero (next);

        if (change == BandChange::continuous)
        {
            if (! next.active)
            {
                in = next;
                return;
            }

            if (asleep)
            {
                if (nextSleeps)
                {
                    in = next;
                    return;
                }

                // Wake: the slot starts at the band's frequency and Q at 0 dB, at rest,
                // then ramps to the gain like any gain move.
                BandSettings zero = next.settings;
                zero.gainDbEff = 0.0;

                const int to = 1 - cur;
                configure (to, zero, next.placement, designer);
                slot[to].setTarget (next.settings);

                ++wakeCount;
                startFade();
                markAsleep (false);
                in = next;
                return;
            }

            slot[cur].setTarget (next.settings);
            in = next;
            return;
        }

        // Structural
        const bool toOff = ! next.active || nextSleeps;

        if (toOff && off[cur])
        {
            in = next;
            markAsleep (nextSleeps);
            return;
        }

        const int to = 1 - cur;

        if (toOff)
            off[to] = true;
        else
            configure (to, next.settings, next.placement, designer);

        startFade();
        in = next;
        markAsleep (nextSleeps);
    }

    /** The fade ends on its target: the outgoing slot is retired and the
        incoming one becomes the current slot. */
    template <typename DesignerType>
    void finishFade (DesignerType& designer) noexcept
    {
        retire (cur, designer);
        cur = 1 - cur;
        stopFade();
    }

    /** finishFade, then the latched request: compared with what the band
        realises at this moment, not with the request that started the fade. */
    template <typename DesignerType>
    void endFade (DesignerType& designer) noexcept
    {
        finishFade (designer);

        if (classifyBandChange (in, desired) != BandChange::identical)
        {
            ++latchedStarted;
            settle (desired, designer);
        }
    }

    //==========================================================================
    Band slot[2];
    Placement place[2] = { Placement::stereo, Placement::stereo };
    bool off[2] = { true, true };

    int cur = 0;                        // the slot that runs with no fade in flight; during a fade, the outgoing one
    bool fading = false;
    int pos = 0;                        // the fade sample the next piece starts on

    BandInput in = inactiveInput();     // what the incoming slot (or the current one) realises
    BandInput desired = inactiveInput(); // the latest request

    bool asleep = false;                // in is an active gain type at exactly 0 dB and the band is off or fading to off
    int zeroTicks = 0;

    unsigned long fadesStarted = 0;
    unsigned long latchedStarted = 0;
    unsigned long sleepCount = 0;
    unsigned long wakeCount = 0;
};

} // namespace eq
