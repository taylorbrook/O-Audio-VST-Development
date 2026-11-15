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

    O-ParametricEQ - band listen (Stage 2, Phase 2.6; PLAN-2.6 P4 - P7;
    RESEARCH-2.6 Q3; CONTEXT D11; ARCHITECTURE section 10 as amended, calls
    C2 and C3)

    Listen solos the listened band's region of the DRY input (the converted
    lanes before any band runs). It sits after auto gain and before output
    gain (C3): the processor multiplies the EQ-path lanes by the auto gain,
    calls process (which works in place on those lanes), then multiplies by
    the output gain (OutputStage::nextGains).

    The mapping (listenInputFor, D11). The listened band's input, effective
    placement already applied, becomes a listen input:

      Bell, Notch, Band Pass   Band Pass at the band's f and Q
      Low Shelf, Low Cut       High Cut (12 dB/oct low-pass): slope index 1, Q 1.0
      High Shelf, High Cut     Low Cut (12 dB/oct high-pass): slope index 1, Q 1.0
      Tilt                     Band Pass at f, Q 0.7

    active is always true (enable and bypass do not stop listen); gain and
    gain scale do not enter; the result is in canonical form (gain 0, Band
    Pass slope index 0). So a gain or slope edit, or a type change inside
    one family (Bell -> Notch), maps to the same listen input and does
    nothing. classifyBandChange on two mapped inputs decides the rest:
    identical (nothing), continuous (frequency, Q: a control ramp on the
    voice), structural (type family, placement: the two-step move below).

    The solo router (P5). F is the voice's filter (an eq::Band), applied to
    the dry lanes; the output replaces the lanes:

      Stereo        L = F(L), R = F(R)        state lanes 0, 1 (processPair)
      Left          L = F(L), R = 0           state lane 0
      Right         L = 0,    R = F(R)        state lane 1
      Mid           m = 0.5 (L + R); L = R = F(m)          state lane 0
      Side          s = 0.5 (L - R); L = F(s), R = -F(s)    state lane 0
      Mono layout   F(x), lane 0 only, whatever the placement

    The input ramp multiplies the FILTER's input: F(r x) per lane for
    Stereo, Left, Right and mono; F(r m) and F(r s) for Mid and Side, m and s
    formed from the unramped dry lanes as above.

    Voices. Two eq::Band voices, each with its placement, its mapped input,
    an input-ramp position and a live flag. `active` is the voice heard when
    no move runs. A voice that is not live is at rest and is not ticked.

    The moves (one at a time; positions count samples from the move's
    first sample; R = the input ramp, N = the fade table's length):

      engage   (target -1 -> band)  the incoming voice (the active one) is
               configured from rest. Move sample p: its input x ramp (p) for
               p < R; then the output is
                   y = eq + w (listen - eq),  w = table.weight (p - R)
               for R <= p < R + N. Before R the output is the EQ path alone
               (weight 0: the lanes are not written). Ends at R + N: fully
               listening.
      change   (band A -> band B; or the listened band's type family or
               placement) the incoming voice B (1 - active) from rest, input
               ramp R; voice A meanwhile at weight 1; then
                   listen = A + w (B - A),  w = table.weight (p - R)
               Ends at R + N: B becomes active, A is retired (reset, not
               live).
      release  (band -> -1) no ramp: the EQ path is warm.
                   y = listen + w (eq - listen),  w = table.weight (p)
               Ends at N: both voices retired, listen idle.
      ramp     (the listened band's f or Q edited) Band::setTarget on the
               active voice, taken up at its next control tick. Not a move:
               no latch, no fade.

    Every mix is a + w (b - a) with a the outgoing signal (Fade.h). The
    ramp is a raised cosine built in prepare with FadeTable::weightOf (i, R)
    (it holds no 0 and no 1); the 20 ms part is the processor's FadeTable.

    Why the weight starts only after the ramp ends. tap_bench listen3
    (48 kHz, 20 listened bands, both tones, 4 phases): a listen filter
    started from rest on a tone in its stop band rings; against its settled
    output (the tone 40 - 90 dB down) that is a burst of up to 29 672x the
    G3 step measure (8 kHz Q 40 on 50 Hz, a 20 ms fade from rest). A
    pre-roll does not cure it: 200 ms still leaves 10 / 3040 band changes
    over 2.0x, because a narrow low-mid band-pass decays slowly (300 Hz
    Q 40: tau = Q / (pi f), about 42 ms). Ramping the filter's input in over
    30 ms removes the step that excites it: R = 30 ms, then the 20 ms fade,
    reads 1.21x on engage and 1.35x on band changes (0 / 3040 over 2.0x),
    1 kHz local kappa 2.36x / 2.41x (the generic level-change kappa of every
    20 ms fade). Overlapping the two does not work: a 50 ms ramp with the
    fade starting at 30 ms reads 30.7x on band changes (42 / 3040 over
    2.0x), 2.87x on engage.

    Why a band change is never a ramp. Moving one band-pass from band A's
    region to band B's sweeps a narrow filter across octaves in 20 ms, a
    chirp: the bench's continuous band-pass -> band-pass ramps read 23 - 76x.
    An index change is always the two-step move, even when the two mapped
    inputs are identical.

    Requests (the SlotBand latch rule). setPending (index, mappedInput) at
    every block start stores the latest wish; applyPending (designer) at the
    next control tick makes it the request (`desired`), so listen renders
    identically at any block size. With no move in flight a request is
    settled at once (the table above). During a move it is LATCHED: it waits
    for the move's end and is then taken as one request, the latest winning,
    compared with what listen realises at that moment (a request back to the
    move's own target starts nothing). One exception, as in SlotBand: during
    an engage or a change, a continuous change of the incoming target (same
    index, f or Q edited) reaches the incoming voice at once (setTarget). A
    move that a latched request starts begins on the next sample.

    Per control tick, in this order: applyPending (designer), then
    controlTick (designer) (the live voices' control ticks).

    Seeding: startSettled (index, input, designer) drops any move, retires
    both voices, and with index >= 0 configures the active voice settled from
    rest with its input ramp complete: weight 1 from the first sample, no
    ramp, no fade. For the first block and the two un-bypass restarts
    (the bypass fade covers the start). goIdle (the fade-to-bypass end):
    voices retired, no move; the target, the request and the pending wish
    are kept, and the restart seeds from them: startSettled must run before
    the next process. clearState (designer) (non-finite input): both
    voices' state cleared and their control ramps snapped; the move and the
    ramp positions carry on.

    Idle: target -1, no move, nothing pending (the pending wish equals the
    request). Idle, process is never called and listen takes no copy.
    isEngaged (target >= 0 or a move in flight) is the part that depends
    only on state changed on the control grid and at move ends: a caller
    that picks a per-segment gain form must pick it from isEngaged, not
    isIdle, or a wish that waits for its tick changes the form at a block
    boundary (block-size dependent; PLAN-2.6 R1). process with
    isEngaged false leaves the lanes untouched.

    Storage: two Bands, the ramp table (11 520 doubles, 90 KB), and per call
    stack pieces of pieceSize samples. No allocation.

    Callers: the processor, the designer harness.

    Standard C++17 only; allocation-free; every function is noexcept.

  ==============================================================================
*/

#pragma once

#include "Band.h"
#include "BandParams.h"
#include "Designer.h"
#include "EqTypes.h"
#include "Fade.h"

#include <cassert>
#include <cmath>

namespace eq
{

//==============================================================================
// Constants
//==============================================================================

inline constexpr double listenRampSeconds = 0.030;

/** 30 ms at 384 kHz: the capacity of the input-ramp table. */
inline constexpr int maxListenRampSamples = 11520;

/** R: the input ramp in samples at this rate. round (0.030 fs), at least 1,
    at most maxListenRampSamples. A rate that is not a positive finite
    number gives 1. */
inline int listenRampLengthSamples (double sampleRateHz) noexcept
{
    if (! std::isfinite (sampleRateHz) || ! (sampleRateHz > 0.0))
        return 1;

    const double samples = listenRampSeconds * sampleRateHz;

    if (samples < 1.0)
        return 1;

    if (samples > static_cast<double> (maxListenRampSamples))
        return maxListenRampSamples;

    return static_cast<int> (std::lround (samples));
}

//==============================================================================
// The mapping (D11)
//==============================================================================

/** The listen input for a band input (canonical, effective placement): see
    the file comment. */
inline BandInput listenInputFor (const BandInput& band) noexcept
{
    BandInput out;
    out.placement = band.placement;
    out.active = true;
    out.settings.freqHz = band.settings.freqHz;
    out.settings.gainDbEff = 0.0;

    switch (band.settings.type)
    {
        case Type::lowShelf:
        case Type::lowCut:
            out.settings.type = Type::highCut;
            out.settings.q = 1.0;
            out.settings.slopeIndex = 1;
            return out;

        case Type::highShelf:
        case Type::highCut:
            out.settings.type = Type::lowCut;
            out.settings.q = 1.0;
            out.settings.slopeIndex = 1;
            return out;

        case Type::tilt:
            out.settings.type = Type::bandPass;
            out.settings.q = 0.7;
            out.settings.slopeIndex = 0;
            return out;

        case Type::bell:
        case Type::notch:
        case Type::bandPass:
            break;
    }

    out.settings.type = Type::bandPass;
    out.settings.q = band.settings.q;
    out.settings.slopeIndex = 0;
    return out;
}

//==============================================================================
// Listen
//==============================================================================

enum class ListenMove : int { none, engage, change, release };

/** Read-only state, for the harness and a processor test accessor. */
struct ListenState
{
    int target;             // -1 or the listened band
    ListenMove move;
    int movePosition;       // move sample the next process call starts on; 0 with no move
    int moveLength;         // R + N (engage, change), N (release), 0 (none)
    int activeVoice;        // the voice heard with no move; the outgoing one in a change
    int incomingVoice;      // engage: the active voice; change: the other; else the active voice
    int rampPosition[2];    // per voice: samples of its input ramp done (R = complete)
    bool voiceLive[2];
    int rampLength;         // R
    int fadeLength;         // N, the fade table's length this was prepared for
    int desiredIndex;       // the request (applied at a tick)
    int pendingIndex;       // the latest wish (setPending)
    bool hasPending;        // the wish differs from the request
};

class Listen
{
public:
    /** The largest piece processed on one set of stack copies. */
    static constexpr int pieceSize = 256;

    Listen() noexcept
    {
        buildRamp (1);
        voice[0].input = inactiveInput();
        voice[1].input = inactiveInput();
        pendingInput = inactiveInput();
        desiredInput = inactiveInput();
    }

    //==========================================================================
    /** Once per prepare: R and its table, N (the FadeTable length at this
        rate: fadeLengthSamples + fadePrerollSamples), K and the control ramp
        for both voices. Both voices retired, no move, target -1, nothing
        requested or pending, counters 0. */
    void prepare (double sampleRateHz, int gridSize, int rampTicks) noexcept
    {
        buildRamp (listenRampLengthSamples (sampleRateHz));
        fadeTotal = fadeLengthSamples (sampleRateHz) + fadePrerollSamples (sampleRateHz);

        for (int v = 0; v < 2; ++v)
        {
            voice[v].band.setControlGrid (gridSize, rampTicks);
            retire (v);
            voice[v].input = inactiveInput();
            voice[v].placement = Placement::stereo;
        }

        active = 0;
        target = -1;
        moveKind = ListenMove::none;
        movePos = 0;

        pendingIndex = -1;
        pendingInput = inactiveInput();
        desiredIndex = -1;
        desiredInput = inactiveInput();

        movesStarted = 0;
        latchedStarted = 0;
    }

    //==========================================================================
    // Requests
    //==========================================================================

    /** At every block start: the listened band (-1 or a negative value: off)
        and its mapped input (listenInputFor (effectiveBandInput (...))).
        Stored only; applyPending takes it at the next control tick. With
        index < 0 the input is ignored. */
    void setPending (int index, const BandInput& mappedInput) noexcept
    {
        if (index < 0)
        {
            pendingIndex = -1;
            pendingInput = inactiveInput();
            return;
        }

        pendingIndex = index;
        pendingInput = mappedInput;
    }

    /** On a control tick, before controlTick: the pending wish becomes the
        request; see the file comment. */
    template <typename DesignerType>
    void applyPending (DesignerType& designer) noexcept
    {
        desiredIndex = pendingIndex;
        desiredInput = pendingInput;

        if (moveKind != ListenMove::none)
        {
            if ((moveKind == ListenMove::engage || moveKind == ListenMove::change)
                && desiredIndex >= 0
                && desiredIndex == target)
            {
                Voice& in = voice[incomingVoice()];

                if (classifyBandChange (in.input, desiredInput) == BandChange::continuous)
                {
                    in.band.setTarget (desiredInput.settings);
                    in.input = desiredInput;
                }
            }

            return;
        }

        settle (designer);
    }

    /** On a control tick, after applyPending: the control tick of every
        live voice. */
    template <typename DesignerType>
    void controlTick (DesignerType& designer) noexcept
    {
        for (int v = 0; v < 2; ++v)
            if (voice[v].live)
                voice[v].band.controlTick (designer);
    }

    //==========================================================================
    // Seeding, idle, non-finite input
    //==========================================================================

    /** Settled on (index, input) at once: no move; index >= 0: the active
        voice configured from rest, input ramp complete, weight 1 from the
        first sample; index < 0: idle. The wish and the request become
        (index, input). */
    template <typename DesignerType>
    void startSettled (int index, const BandInput& input, DesignerType& designer) noexcept
    {
        setPending (index, input);
        desiredIndex = pendingIndex;
        desiredInput = pendingInput;

        moveKind = ListenMove::none;
        movePos = 0;
        retire (0);
        retire (1);

        if (desiredIndex >= 0)
        {
            configure (active, desiredInput, designer);
            voice[active].rampPos = rampLength;
            target = desiredIndex;
        }
        else
        {
            target = -1;
        }
    }

    /** The fade-to-bypass end: both voices retired, no move. The target, the
        request and the wish are kept (startSettled seeds the restart). */
    void goIdle() noexcept
    {
        moveKind = ListenMove::none;
        movePos = 0;
        retire (0);
        retire (1);
    }

    /** Non-finite input: both voices' filter state cleared and their control
        ramps snapped (reset + snapToTarget). The move, the ramp positions and
        the live flags carry on. */
    template <typename DesignerType>
    void clearState (DesignerType& designer) noexcept
    {
        for (int v = 0; v < 2; ++v)
        {
            voice[v].band.reset();
            voice[v].band.snapToTarget (designer);
        }
    }

    //==========================================================================
    // Queries
    //==========================================================================

    /** The pending wish differs from the request. */
    bool hasPending() const noexcept
    {
        if (pendingIndex != desiredIndex)
            return true;

        return pendingIndex >= 0 && ! sameBandInput (pendingInput, desiredInput);
    }

    /** Listening, or a move in flight: process writes the lanes. */
    bool isEngaged() const noexcept                     { return target >= 0 || moveKind != ListenMove::none; }

    /** Target -1, no move, nothing pending. */
    bool isIdle() const noexcept                        { return ! isEngaged() && ! hasPending(); }

    /** Samples to the end of the move in flight; 0 with none. A caller may
        end its segments there. */
    int samplesLeftInMove() const noexcept
    {
        if (moveKind == ListenMove::none)
            return 0;

        const int left = moveLength() - movePos;
        return (left > 0) ? left : 0;
    }

    int getTarget() const noexcept                      { return target; }
    ListenMove getMove() const noexcept                 { return moveKind; }
    int getRampLength() const noexcept                  { return rampLength; }
    int getFadeLength() const noexcept                  { return fadeTotal; }
    int getPendingIndex() const noexcept                { return pendingIndex; }
    const BandInput& getPendingInput() const noexcept   { return pendingInput; }
    int getDesiredIndex() const noexcept                { return desiredIndex; }
    const BandInput& getDesiredInput() const noexcept   { return desiredInput; }

    /** Voice `index` (0 or 1; any other value reads voice 0), read only. */
    const Band& getVoiceBand (int index) const noexcept         { return voice[(index == 1) ? 1 : 0].band; }
    const BandInput& getVoiceInput (int index) const noexcept   { return voice[(index == 1) ? 1 : 0].input; }
    Placement getVoicePlacement (int index) const noexcept      { return voice[(index == 1) ? 1 : 0].placement; }

    /** Since the last prepare: moves started (all kinds), and move ends
        that found the request different from what listen realised and
        settled on it. */
    unsigned long getMovesStarted() const noexcept      { return movesStarted; }
    unsigned long getLatchedStarted() const noexcept    { return latchedStarted; }

    ListenState getState() const noexcept
    {
        ListenState s;
        s.target = target;
        s.move = moveKind;
        s.movePosition = movePos;
        s.moveLength = (moveKind == ListenMove::none) ? 0 : moveLength();
        s.activeVoice = active;
        s.incomingVoice = incomingVoice();
        s.rampPosition[0] = voice[0].rampPos;
        s.rampPosition[1] = voice[1].rampPos;
        s.voiceLive[0] = voice[0].live;
        s.voiceLive[1] = voice[1].live;
        s.rampLength = rampLength;
        s.fadeLength = fadeTotal;
        s.desiredIndex = desiredIndex;
        s.pendingIndex = pendingIndex;
        s.hasPending = hasPending();
        return s;
    }

    //==========================================================================
    // Processing
    //==========================================================================

    /** In place on the EQ-path lanes L, R (already x auto gain), with the
        dry lanes dryL, dryR (the converted input before any band ran), n
        samples. stereo is the negotiated layout: mono reads and writes lane
        0 only (dryR and R may be null). `table` is the processor's fade
        table, prepared at the rate this was prepared at. A move that ends
        inside the call ends there; a latched request then starts at the
        next sample, in the same call. Not engaged: nothing is touched. */
    template <typename DesignerType>
    void process (const double* dryL, const double* dryR, double* L, double* R, int n, bool stereo,
                  const FadeTable& table, DesignerType& designer) noexcept
    {
        assert (table.getLength() == fadeTotal);

        const int lanes = stereo ? 2 : 1;
        int done = 0;

        while (done < n)
        {
            if (! isEngaged())
                return;

            int count = n - done;

            if (count > pieceSize)
                count = pieceSize;

            if (moveKind != ListenMove::none)
            {
                const int left = moveLength() - movePos;

                if (count > left)
                    count = left;
            }

            const double* dry[2] = { dryL + done, stereo ? dryR + done : nullptr };
            double* io[2] = { L + done, stereo ? R + done : nullptr };

            double a[2][pieceSize];
            double b[2][pieceSize];

            switch (moveKind)
            {
                case ListenMove::none:
                {
                    assert (voice[active].live);
                    renderVoice (active, dry[0], dry[1], a[0], a[1], count, stereo);

                    for (int l = 0; l < lanes; ++l)
                        for (int i = 0; i < count; ++i)
                            io[l][i] = a[l][i];

                    break;
                }

                case ListenMove::engage:
                {
                    renderVoice (active, dry[0], dry[1], b[0], b[1], count, stereo);

                    for (int l = 0; l < lanes; ++l)
                        for (int i = 0; i < count; ++i)
                        {
                            const int p = movePos + i;

                            if (p < rampLength)
                                continue;

                            const double w = table.weight (p - rampLength);
                            const double x = io[l][i];
                            io[l][i] = x + w * (b[l][i] - x);
                        }

                    break;
                }

                case ListenMove::change:
                {
                    renderVoice (active, dry[0], dry[1], a[0], a[1], count, stereo);
                    renderVoice (1 - active, dry[0], dry[1], b[0], b[1], count, stereo);

                    for (int l = 0; l < lanes; ++l)
                        for (int i = 0; i < count; ++i)
                        {
                            const int p = movePos + i;
                            const double x = a[l][i];

                            if (p < rampLength)
                            {
                                io[l][i] = x;
                                continue;
                            }

                            const double w = table.weight (p - rampLength);
                            io[l][i] = x + w * (b[l][i] - x);
                        }

                    break;
                }

                case ListenMove::release:
                {
                    renderVoice (active, dry[0], dry[1], a[0], a[1], count, stereo);

                    for (int l = 0; l < lanes; ++l)
                        for (int i = 0; i < count; ++i)
                        {
                            const double w = table.weight (movePos + i);
                            const double x = a[l][i];
                            io[l][i] = x + w * (io[l][i] - x);
                        }

                    break;
                }
            }

            done += count;

            if (moveKind != ListenMove::none)
            {
                movePos += count;

                if (movePos >= moveLength())
                    endMove (designer);
            }
        }
    }

    /** The solo router of the file comment, for one filter: outL / outR
        receive F of the dry lanes by placement; inputGain (may be null)
        multiplies the filter's input per sample. Mono: lane 0 only. */
    static void soloRender (Band& band, Placement placement, const double* dryL, const double* dryR,
                            double* outL, double* outR, int n, bool stereo, const double* inputGain) noexcept
    {
        if (n <= 0)
            return;

        if (! stereo)
        {
            for (int i = 0; i < n; ++i)
                outL[i] = gained (dryL[i], inputGain, i);

            band.processLane (0, outL, n);
            return;
        }

        switch (placement)
        {
            case Placement::left:
                for (int i = 0; i < n; ++i)
                {
                    outL[i] = gained (dryL[i], inputGain, i);
                    outR[i] = 0.0;
                }

                band.processLane (0, outL, n);
                return;

            case Placement::right:
                for (int i = 0; i < n; ++i)
                {
                    outL[i] = 0.0;
                    outR[i] = gained (dryR[i], inputGain, i);
                }

                band.processLane (1, outR, n);
                return;

            case Placement::mid:
                for (int i = 0; i < n; ++i)
                    outL[i] = gained (0.5 * (dryL[i] + dryR[i]), inputGain, i);

                band.processLane (0, outL, n);

                for (int i = 0; i < n; ++i)
                    outR[i] = outL[i];

                return;

            case Placement::side:
                for (int i = 0; i < n; ++i)
                    outL[i] = gained (0.5 * (dryL[i] - dryR[i]), inputGain, i);

                band.processLane (0, outL, n);

                for (int i = 0; i < n; ++i)
                    outR[i] = -outL[i];

                return;

            case Placement::stereo:
                break;
        }

        for (int i = 0; i < n; ++i)
        {
            outL[i] = gained (dryL[i], inputGain, i);
            outR[i] = gained (dryR[i], inputGain, i);
        }

        band.processPair (outL, outR, n);
    }

private:
    struct Voice
    {
        Band band;
        Placement placement = Placement::stereo;
        BandInput input;
        int rampPos = 0;        // samples of the input ramp done; rampLength = complete
        bool live = false;
    };

    static double gained (double x, const double* gain, int i) noexcept
    {
        return (gain != nullptr) ? x * gain[i] : x;
    }

    static BandInput inactiveInput() noexcept
    {
        BandInput input;
        input.settings = BandSettings { Type::bell, 1000.0, 0.0, 1.0, 0 };
        input.placement = Placement::stereo;
        input.active = false;
        return input;
    }

    void buildRamp (int length) noexcept
    {
        int count = length;

        if (count < 1)
            count = 1;

        if (count > maxListenRampSamples)
            count = maxListenRampSamples;

        rampLength = count;

        for (int i = 0; i < count; ++i)
            ramp[i] = FadeTable::weightOf (i, count);

        // Past the ramp the input is used as it is; these entries are never read.
        for (int i = count; i < maxListenRampSamples; ++i)
            ramp[i] = 1.0;
    }

    int moveLength() const noexcept
    {
        switch (moveKind)
        {
            case ListenMove::engage:
            case ListenMove::change:    return rampLength + fadeTotal;
            case ListenMove::release:   return fadeTotal;
            case ListenMove::none:      break;
        }

        return 0;
    }

    int incomingVoice() const noexcept
    {
        return (moveKind == ListenMove::change) ? 1 - active : active;
    }

    /** Voice v through the solo router on its own input ramp, into outL /
        outR; the ramp position moves on by count. */
    void renderVoice (int v, const double* dryL, const double* dryR, double* outL, double* outR,
                      int count, bool stereo) noexcept
    {
        Voice& vc = voice[v];
        double gain[pieceSize];
        const double* g = nullptr;

        if (vc.rampPos < rampLength)
        {
            for (int i = 0; i < count; ++i)
            {
                const int p = vc.rampPos + i;
                gain[i] = (p < rampLength) ? ramp[p] : 1.0;
            }

            g = gain;
        }

        soloRender (vc.band, vc.placement, dryL, dryR, outL, outR, count, stereo, g);

        const int next = vc.rampPos + count;
        vc.rampPos = (next < rampLength) ? next : rampLength;
    }

    /** From rest, designed settled from `input`; input ramp at 0; live. */
    template <typename DesignerType>
    void configure (int v, const BandInput& input, DesignerType& designer) noexcept
    {
        voice[v].band.snapTo (input.settings, designer);
        voice[v].band.reset();
        voice[v].placement = input.placement;
        voice[v].input = input;
        voice[v].rampPos = 0;
        voice[v].live = true;
    }

    /** At rest and not live. The design and the control ramp are left as
        they are: configure snaps both before the voice is heard again. */
    void retire (int v) noexcept
    {
        voice[v].band.reset();
        voice[v].rampPos = 0;
        voice[v].live = false;
    }

    void startMove (ListenMove kind) noexcept
    {
        moveKind = kind;
        movePos = 0;
        ++movesStarted;
    }

    /** No move in flight: brings listen to the request. */
    template <typename DesignerType>
    void settle (DesignerType& designer) noexcept
    {
        if (desiredIndex < 0)
        {
            if (target >= 0)
            {
                target = -1;
                startMove (ListenMove::release);
            }

            return;
        }

        if (target < 0)
        {
            configure (active, desiredInput, designer);
            target = desiredIndex;
            startMove (ListenMove::engage);
            return;
        }

        if (desiredIndex != target)
        {
            configure (1 - active, desiredInput, designer);
            target = desiredIndex;
            startMove (ListenMove::change);
            return;
        }

        Voice& cur = voice[active];

        switch (classifyBandChange (cur.input, desiredInput))
        {
            case BandChange::identical:
                return;

            case BandChange::continuous:
                cur.band.setTarget (desiredInput.settings);
                cur.input = desiredInput;
                return;

            case BandChange::structural:
                configure (1 - active, desiredInput, designer);
                startMove (ListenMove::change);
                return;
        }
    }

    /** True when the request differs from what listen realises. */
    bool requestDiffers() const noexcept
    {
        if (desiredIndex != target)
            return true;

        return target >= 0 && classifyBandChange (voice[active].input, desiredInput) != BandChange::identical;
    }

    /** The move in flight has run its length. */
    template <typename DesignerType>
    void endMove (DesignerType& designer) noexcept
    {
        switch (moveKind)
        {
            case ListenMove::change:
                retire (active);
                active = 1 - active;
                break;

            case ListenMove::release:
                retire (0);
                retire (1);
                break;

            case ListenMove::engage:
            case ListenMove::none:
                break;
        }

        moveKind = ListenMove::none;
        movePos = 0;

        if (requestDiffers())
        {
            ++latchedStarted;
            settle (designer);
        }
    }

    //==========================================================================
    Voice voice[2];
    int active = 0;
    int target = -1;
    ListenMove moveKind = ListenMove::none;
    int movePos = 0;

    int pendingIndex = -1;
    BandInput pendingInput;
    int desiredIndex = -1;
    BandInput desiredInput;

    int rampLength = 1;
    int fadeTotal = 1;
    double ramp[maxListenRampSamples];

    unsigned long movesStarted = 0;
    unsigned long latchedStarted = 0;
};

} // namespace eq
