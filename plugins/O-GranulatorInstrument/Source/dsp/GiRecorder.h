/*
   This file is part of O-GranulatorInstrument, an Ouaricon Audio plugin.
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
#include <juce_audio_processors/juce_audio_processors.h>
#include <algorithm>
#include <atomic>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>

#include "GiConstants.h"
#include "forked/ControlGrid.h"    // ExactRamp, dbToGain (snapDb), rampCellsFor
#include "forked/TempoTracker.h"

// ============================================================================
// GiRecorder — Record-in capture (CONTEXT-2.6 D34 / D35 / Defaults,
// PLAN-2.6 T6, PD5 / PD6 / PD9 / PD10, RESEARCH-2.6 §2–§4).
//
// States Idle → Armed → Recording → Committing → Idle (engine state, never a
// parameter). The 7 rec_* parameters are read as raw atomics here (PD18; no
// ControlGrid edit):
//   * rec_arm / rec_target / rec_trigger once at the top of each host block;
//     the arm EDGE is stamped at the block start (P13). One-shot (D34): only an
//     Off→On edge arms; after a commit / discard the state stays Idle while the
//     arm is still On. On→Off: Armed → Cancelled; Recording → stop (≥ 0.5 s and
//     present → commit, else discard). An edge during Committing is HELD and
//     goes Armed at the first block start after the copy-out.
//     Auto-clear (CONTEXT-R1 D66, PLAN-R1 PD18 / PD19): the one-shot rule above
//     is unchanged; the recorder only REPORTS that an arm is over, and the
//     processor's 20 ms message-thread timer writes rec_arm back to Off (never
//     this class, never a listener, never the audio thread). Two relaxed
//     generation counters: armGen moves on every accepted On edge (Idle: armed
//     or no sidechain bus; Committing: held); endGen = armGen at every ending
//     that leaves the parameter On: commit, NoSignal, TooShort, prepare() while
//     Armed / Recording, bypass while Armed / Recording, no sidechain bus, the
//     capacity overrun, and an On that prepare() finds with no take behind it
//     (the edge was swallowed: armGen moves, then ends). Not on Cancelled (the
//     user wrote Off). A waiting arm (Threshold, Next Note) writes nothing. The
//     timer clears only while endGen == armGen, so the clear of take N cannot
//     cancel an arm N + 1 held during Committing; that arm's own ending is
//     stamped N + 1 (enterArmed does not move armGen).
//   * rec_threshold / rec_gain at grid ticks (gain = a 20 ms ExactRamp through
//     ControlGrid's dbToGain, so the 7e-7 dB default is exactly unity, P11).
//   * rec_length / rec_length_sync / sync_mode at the start sample: bars only
//     under sync_mode = Tempo AND a playing host (bar length from the tracker
//     anchor), seconds otherwise; capped at F_max (fs) = min (30 s, 2.88 M).
//   * target + trigger latch at Armed entry.
// Triggers: Manual starts at the arm sample; Threshold starts 10 ms before the
// first sample with max |x| > thr (clipped at the arm sample; the pre-roll
// ring runs only while Armed in Threshold and counts toward the length); Next
// Note starts at the first serial-minting note event (Engine::startVoice /
// retriggerVoice). x = sc · g (float); a mono sidechain is captured mono (PD6).
// D6 presence: a take whose running peak is < −90 dBFS is discarded (NoSignal);
// one shorter than ceil (0.5 · fs) is discarded (TooShort).
// Commit (PD7): the audio thread fills `commit`, then ready.store (frames,
// release) — nothing else. An off-audio poll copies the buffer out under
// captureLock (takeCommit) and clears `ready`. The audio thread never locks or
// allocates; the buffer only grows in prepare(), under captureLock, and never
// while a copy-out is pending (P12).
// ============================================================================
namespace ogi
{
    enum class RecState : std::uint8_t { Idle = 0, Armed = 1, Recording = 2, Committing = 3 };
    enum class RecReason : std::uint8_t
    {
        None = 0, SidechainUnavailable, NoSignal, TooShort, Cancelled, Interrupted, Bypassed, Committed, WavFailed
    };
    enum class RecTrigger : int { Manual = 0, Threshold = 1, NextNote = 2 };

    /** D66 (PLAN-R1 PD19): the seven statements that end an arm with the
        parameter still On (endGen = armGen). The table rows of RESEARCH-R1
        D·Q9 they cover:
          Committed            1 (and 7 / 8 / 10 when the stopped take commits)
          NoSignal             2 (and 7 / 8 / 10 when the stopped take is silent)
          TooShort             3 (reached through 5, 7, 8)
          Interrupted          6  prepare() while Armed
          Bypassed             8  bypass while Armed
          SidechainUnavailable 9  an On edge with no sidechain bus
          StaleOn              11 prepare() finds On with no take behind it
        The values are what RecorderTestHooks::skipEndGen names. */
    enum class RecEnding : int
    {
        None = 0, Committed = 1, NoSignal = 2, TooShort = 3, Interrupted = 4, Bypassed = 5,
        SidechainUnavailable = 6, StaleOn = 7
    };

    /** The commit record the audio thread hands over with `ready`. */
    struct RecCommit
    {
        int          slot     = 0;
        int          frames   = 0;
        double       rate     = 0.0;   // the capture rate (fs at the take; PD9: fs_old after a prepare)
        int          channels = 0;
        std::int64_t startAbs = 0;     // absolute engine sample of capture frame 0
    };

   #if OGI_TEST_HOOKS
    struct RecorderTestHooks
    {
        // switches (PLAN-2.6 PD23)
        bool startOffset     = false;   // RC1n: the take starts one sample late
        bool prerollOff      = false;   // TH1n: no Threshold pre-roll
        bool armLevelTrigger = false;   // OS1n: arm by level (re-arms while held On)
        bool capOff          = false;   // LN4n: the length is not capped at F_max (an overrun is counted, dropped)
        bool nnAnyNote       = false;   // MN1n: a Legato pitch move counts as Next Note
        int  skipEndGen      = 0;       // AC2n / AC7n (PLAN-R1 PD31): the RecEnding (1…7) whose endGen write is removed; 0 = none
        // counters
        std::uint64_t recArms = 0, recStarts = 0, recCommits = 0, recDiscards = 0, recCancels = 0, heldArms = 0;
        std::uint64_t overruns = 0, unavailable = 0, interrupted = 0, bypassed = 0;
        std::uint64_t armEnds = 0, staleArms = 0;   // D66: endGen writes; StaleOn endings reached (counted before the skip)
        void resetCounters() noexcept
        {
            recArms = recStarts = recCommits = recDiscards = recCancels = heldArms = 0;
            overruns = unavailable = interrupted = bypassed = 0;
            armEnds = staleArms = 0;
        }
    };

    /** A probe of the recorder (harness thread, between blocks). */
    struct RecorderProbe
    {
        RecState     state = RecState::Idle;
        RecReason    reason = RecReason::None;
        bool         heldArm = false;
        std::int64_t armAt = -1, start = -1, stop = -1;
        float        peak = 0.0f;
        int          frames = 0, target = 0, capFrames = 0, capacity = 0, channels = 0, slot = 0, preroll = 0;
        RecTrigger   trigger = RecTrigger::Manual;
    };
   #endif

    class GiRecorder
    {
    public:
        static constexpr float  kPresenceLin = 3.16227766e-5f;   // −90 dBFS (D6)
        static constexpr double kMinSeconds  = 0.5;              // ARCH §22
        static constexpr double kPrerollSec  = 0.010;            // D35

        /** F_max (fs) = min (⌊30 s · fs⌋, kSlotMaxFrames). */
        static int maxFramesFor (double fs) noexcept
        {
            return static_cast<int> (std::min (std::floor (kSlotMaxSeconds * fs), static_cast<double> (kSlotMaxFrames)));
        }

        /** The UNCAPPED take length in frames (≥ 1), the one formula of the
            recorder (startTake) and of the UI's `rec.capped` (PLAN-3.2 PD11).
            tempo: rec_length_sync index `barsIndex` (clamped 0…5 → 1, 2, 4, 8,
            16, 32 bars) × the bar in samples (barLenQuarters / pps; pps =
            quarters per sample = bpm / 60 / fs; 0 when pps ≤ 0). Otherwise
            `seconds` × sampleRate. The caller applies the F_max cap. */
        static std::int64_t takeLengthFrames (bool tempo, int barsIndex, double barLenQuarters, double pps,
                                              double seconds, double sampleRate) noexcept
        {
            double len;
            if (tempo)
            {
                static constexpr int kBars[6] = { 1, 2, 4, 8, 16, 32 };
                const int bi = std::clamp (barsIndex, 0, 5);
                const double barSamples = pps > 0.0 ? barLenQuarters / pps : 0.0;
                len = static_cast<double> (kBars[bi]) * barSamples;
            }
            else
            {
                len = seconds * sampleRate;
            }
            return std::max<std::int64_t> (1, std::llround (len));
        }

        /** Message thread (processor constructor via Engine::bind). */
        void bind (juce::AudioProcessorValueTreeState& apvts)
        {
            pArm       = apvts.getRawParameterValue ("rec_arm");
            pTarget    = apvts.getRawParameterValue ("rec_target");
            pTrigger   = apvts.getRawParameterValue ("rec_trigger");
            pThreshold = apvts.getRawParameterValue ("rec_threshold");
            pLength    = apvts.getRawParameterValue ("rec_length");
            pLenSync   = apvts.getRawParameterValue ("rec_length_sync");
            pGain      = apvts.getRawParameterValue ("rec_gain");
            pSyncMode  = apvts.getRawParameterValue ("sync_mode");
            jassert (pArm != nullptr && pTarget != nullptr && pTrigger != nullptr && pThreshold != nullptr
                     && pLength != nullptr && pLenSync != nullptr && pGain != nullptr && pSyncMode != nullptr);
        }

        /** Host thread (Engine::prepare; process() not running). PD9: a take in
            flight stops (commit at the OLD rate) or an Armed state is
            interrupted; Committing survives; the buffer grows (never shrinks,
            never while a copy-out is pending). */
        void prepare (double sampleRate)
        {
            // D66: an arm this prepare() ends is stamped by its own ending
            // (stopTake / Interrupted), never a second time as StaleOn below.
            const bool endedHere = state == RecState::Recording || state == RecState::Armed;
            if (state == RecState::Recording)
            {
                stopTake (lastSample);
            }
            else if (state == RecState::Armed)
            {
                toIdle (RecReason::Interrupted);
                endArm (RecEnding::Interrupted);
            }
           #if OGI_TEST_HOOKS
            if (state == RecState::Idle && reason == RecReason::Interrupted)
                ++hooks.interrupted;
           #endif

            fs = sampleRate > 0.0 ? sampleRate : 44100.0;
            const int need = maxFramesFor (fs);
            {
                const juce::ScopedLock sl (captureLock);
                if (ready.load (std::memory_order_acquire) == 0 && capacity < need)
                {
                    cap[0].reset (new float[(size_t) need]);   // uninitialised: pages commit as the take writes
                    cap[1].reset (new float[(size_t) need]);
                    capacity = need;
                }
            }
            capFrames = std::min (need, capacity);

            preroll = std::max (1, static_cast<int> (std::lround (kPrerollSec * fs)));
            if (preroll > ringCap)
            {
                ring[0].reset (new float[(size_t) preroll]);
                ring[1].reset (new float[(size_t) preroll]);
                ringCap = preroll;
            }
            ringCount = ringPos = 0;
            minFrames = static_cast<int> (std::ceil (kMinSeconds * fs));
            rampCells = rampCellsFor (fs);
            gain.reset (dbToGain (load (pGain, 0.0f)));
            thrLin = juce::Decibels::decibelsToGain (load (pThreshold, -30.0f));
            prevArm = load (pArm, 0.0f) > 0.5f;   // an arm held On never re-arms by itself
            // D66 row 11: an On with no take behind it (Idle, or Committing
            // with nothing held) can never start: the seed above swallows its
            // edge. It gets a generation of its own, ended at once. A held arm
            // is a real arm (it goes Armed after the copy-out) and is left.
            if (prevArm && ! endedHere && ! heldArm)
            {
                armGen.fetch_add (1, std::memory_order_relaxed);
               #if OGI_TEST_HOOKS
                ++hooks.staleArms;
               #endif
                endArm (RecEnding::StaleOn);
            }
            publish();
        }

        //======================================================================
        // Audio thread.

        /** Top of each host block (after the tracker update). */
        void beginBlock (std::int64_t blockStart, int scChannels, const TempoTracker& tracker) noexcept
        {
            if (wavFailedFlag.exchange (false, std::memory_order_acq_rel))
                reason = RecReason::WavFailed;

            if (state == RecState::Committing && ready.load (std::memory_order_acquire) == 0)
            {
                state = RecState::Idle;
                if (heldArm)
                {
                    heldArm = false;
                    enterArmed (blockStart, scChannels, tracker);
                }
            }

            const bool arm = load (pArm, 0.0f) > 0.5f;
            bool on = arm && ! prevArm;
            const bool off = ! arm && prevArm;
           #if OGI_TEST_HOOKS
            if (hooks.armLevelTrigger && arm && state == RecState::Idle)
                on = true;
           #endif
            prevArm = arm;

            if (on)
            {
                if (state == RecState::Idle)
                {
                    armGen.fetch_add (1, std::memory_order_relaxed);   // D66: an accepted On edge
                    if (scChannels <= 0)
                    {
                        reason = RecReason::SidechainUnavailable;   // no bus: the arm is inert
                       #if OGI_TEST_HOOKS
                        ++hooks.unavailable;
                       #endif
                        endArm (RecEnding::SidechainUnavailable);
                    }
                    else
                    {
                        enterArmed (blockStart, scChannels, tracker);
                    }
                }
                else if (state == RecState::Committing)
                {
                    armGen.fetch_add (1, std::memory_order_relaxed);   // D66: the held arm is newer than the take being copied out
                    heldArm = true;   // held, not dropped
                   #if OGI_TEST_HOOKS
                    ++hooks.heldArms;
                   #endif
                }
            }
            else if (off)
            {
                if (state == RecState::Armed)
                {
                    toIdle (RecReason::Cancelled);
                   #if OGI_TEST_HOOKS
                    ++hooks.recCancels;
                   #endif
                }
                else if (state == RecState::Recording)
                {
                    stopTake (blockStart);
                }
                else if (state == RecState::Committing)
                {
                    heldArm = false;
                }
            }
            publish();
        }

        /** At each grid tick: threshold + gain latch (PD5). */
        void gridTick() noexcept
        {
            thrLin = juce::Decibels::decibelsToGain (load (pThreshold, -30.0f));
            gain.setTarget (dbToGain (load (pGain, 0.0f)), rampCells);
            gain.tick();
        }

        /** A serial-minting note event at absolute sample t (Next Note). */
        void noteMinted (std::int64_t t) noexcept
        {
            if (state == RecState::Armed && trigger == RecTrigger::NextNote && nnAt < 0)
                nnAt = t;
        }

        /** Per piece [c0, c1) of the cell (c = cell offset), t0 = the absolute
            sample of c0. sc = this cell's sidechain scratch. Absolute-sample
            ordered, so block-size invariant. */
        void runPiece (int c0, int c1, std::int64_t t0, const float (&sc)[2][kCell], const TempoTracker& tracker) noexcept
        {
            // PLAN-3.3 PD5: the meter peaks per side (telemetry only); `a`
            // (threshold, capture peak) is unchanged. Mono: xR == xL.
            float pkL = 0.0f, pkR = 0.0f;
            for (int c = c0; c < c1; ++c)
            {
                const std::int64_t t = t0 + (c - c0);
                const float g  = gain.value (c);
                const float xL = sc[0][c] * g;
                const float xR = channels > 1 ? sc[1][c] * g : xL;
                const float a  = std::max (std::abs (xL), std::abs (xR));
                pkL = std::max (pkL, std::abs (xL));
                pkR = std::max (pkR, std::abs (xR));
                lastSample = t + 1;

                if (state == RecState::Armed)
                {
                    if (trigger == RecTrigger::Threshold)
                    {
                        if (a > thrLin)
                        {
                            int pre = std::min (ringCount, preroll);
                           #if OGI_TEST_HOOKS
                            if (hooks.prerollOff)
                                pre = 0;
                           #endif
                            startTake (t, pre, tracker);
                        }
                        else
                        {
                            ring[0][(size_t) ringPos] = xL;
                            ring[1][(size_t) ringPos] = xR;
                            ringPos = (ringPos + 1) % preroll;
                            ringCount = std::min (ringCount + 1, preroll);
                            continue;
                        }
                    }
                    else if (trigger == RecTrigger::NextNote && nnAt >= 0 && t >= nnAt)
                    {
                        startTake (t, 0, tracker);
                    }
                    else
                    {
                        continue;
                    }
                }

                if (state == RecState::Recording)
                {
                    if (skip > 0)
                    {
                        --skip;   // RC1n (startOffset): the first sample is not captured
                        continue;
                    }
                    if (frames >= capacity)
                    {
                       #if OGI_TEST_HOOKS
                        ++hooks.overruns;   // capOff only: the length outran the buffer
                       #endif
                        stopTake (t);
                        continue;
                    }
                    cap[0][(size_t) frames] = xL;
                    if (channels > 1)
                        cap[1][(size_t) frames] = xR;
                    peak = std::max (peak, a);
                    ++frames;
                    if (frames >= targetFrames)
                        stopTake (t + 1);
                }
            }
            scPeakPush (scPeakL, pkL);
            scPeakPush (scPeakR, pkR);
            framesTel.store (frames, std::memory_order_relaxed);
            publish();
        }

        /** PD10: the host bypassed this block. */
        void bypassedBlock() noexcept
        {
            if (state == RecState::Armed)
            {
                toIdle (RecReason::Bypassed);
                endArm (RecEnding::Bypassed);
               #if OGI_TEST_HOOKS
                ++hooks.bypassed;
               #endif
            }
            else if (state == RecState::Recording)
            {
                stopTake (lastSample);
            }
            publish();
        }

        //======================================================================
        // Off-audio (the processor's poll / the harness commitSync).

        /** Copies a ready take out under captureLock, then frees the buffer
            (ready = 0). False when nothing is ready. Allocates (caller thread). */
        bool takeCommit (juce::AudioBuffer<float>& out, RecCommit& c)
        {
            const juce::ScopedLock sl (captureLock);
            const int f = ready.load (std::memory_order_acquire);
            if (f <= 0)
                return false;
            c = commit;
            out.setSize (std::max (1, c.channels), f, false, false, false);
            for (int ch = 0; ch < out.getNumChannels(); ++ch)
                std::memcpy (out.getWritePointer (ch), cap[(size_t) ch].get(), sizeof (float) * (size_t) f);
            ready.store (0, std::memory_order_release);
            return true;
        }

        bool commitPending() const noexcept { return ready.load (std::memory_order_acquire) > 0; }

        /** The worker could not write the WAV (any thread; lock-free). */
        void noteWavFailed() noexcept { wavFailedFlag.store (true, std::memory_order_release); }

        // Telemetry (relaxed; any thread). recState = state | reason << 8 | heldArm << 16.
        std::uint32_t getPackedState() const noexcept   { return packed.load (std::memory_order_relaxed); }
        int  getFramesTelemetry() const noexcept        { return framesTel.load (std::memory_order_relaxed); }
        int  getTargetTelemetry() const noexcept        { return targetTel.load (std::memory_order_relaxed); }
        int  getTargetSlotTelemetry() const noexcept    { return slotTel.load (std::memory_order_relaxed); }
        /** D66 (relaxed; any thread). A reader that compares them reads endGen
            FIRST: armGen only grows, so a held arm that lands between the two
            reads makes them differ (no clear), never agree. */
        std::uint32_t getArmGen() const noexcept        { return armGen.load (std::memory_order_relaxed); }
        std::uint32_t getEndGen() const noexcept        { return endGen.load (std::memory_order_relaxed); }
        /** PLAN-3.3 PD5: max post-gain |sc L| and |sc R| since the last read
            (the reader takes both; ONE caller, the processor's
            buildUiTelemetry). Mono sidechain: both sides carry the L sample. */
        void takeScPeaks (float& l, float& r) noexcept
        {
            l = scPeakL.exchange (0.0f, std::memory_order_acq_rel);
            r = scPeakR.exchange (0.0f, std::memory_order_acq_rel);
        }

       #if OGI_TEST_HOOKS
        /** PLAN-3.3 UMT4n control: the peaks WITHOUT the exchange. */
        void peekScPeaksForTest (float& l, float& r) const noexcept
        {
            l = scPeakL.load (std::memory_order_relaxed);
            r = scPeakR.load (std::memory_order_relaxed);
        }
        RecorderTestHooks hooks;
        RecorderProbe probe() const noexcept
        {
            RecorderProbe p;
            p.state = state; p.reason = reason; p.heldArm = heldArm;
            p.armAt = armAt; p.start = startAbs; p.stop = stopAbs; p.peak = peak;
            p.frames = frames; p.target = targetFrames; p.capFrames = capFrames; p.capacity = capacity;
            p.channels = channels; p.slot = slot; p.preroll = preroll; p.trigger = trigger;
            return p;
        }
        /** The captured sample i of channel ch (the live buffer). */
        float captureAt (int ch, int i) const noexcept
        {
            return (i >= 0 && i < capacity && ch >= 0 && ch < 2 && cap[(size_t) ch] != nullptr) ? cap[(size_t) ch][(size_t) i] : 0.0f;
        }
        float gainNow() const noexcept { return gain.current(); }
       #endif

    private:
        static float load (const std::atomic<float>* p, float fallback) noexcept
        {
            return p != nullptr ? p->load (std::memory_order_relaxed) : fallback;
        }

        void enterArmed (std::int64_t blockStart, int scChannels, const TempoTracker& tracker) noexcept
        {
            state    = RecState::Armed;
            reason   = RecReason::None;
            armAt    = blockStart;
            slot     = std::clamp (static_cast<int> (std::lround (load (pTarget, 0.0f))), 0, kTapeSlots - 1);
            trigger  = static_cast<RecTrigger> (std::clamp (static_cast<int> (std::lround (load (pTrigger, 0.0f))), 0, 2));
            channels = std::clamp (scChannels, 1, 2);
            ringCount = ringPos = 0;
            nnAt = -1;
            frames = 0;
            peak = 0.0f;
            startAbs = stopAbs = -1;
           #if OGI_TEST_HOOKS
            ++hooks.recArms;
           #endif
            if (trigger == RecTrigger::Manual)
                startTake (blockStart, 0, tracker);   // sample-exact: the arm sample
        }

        /** Enters Recording at sample t with `pre` pre-roll frames already in the ring. */
        void startTake (std::int64_t t, int pre, const TempoTracker& tracker) noexcept
        {
            frames = 0;
            peak = 0.0f;
            if (pre > 0)
            {
                // The last `pre` ring samples, oldest first.
                int idx = (ringPos - pre + preroll) % preroll;
                for (int i = 0; i < pre; ++i)
                {
                    cap[0][(size_t) i] = ring[0][(size_t) idx];
                    if (channels > 1)
                        cap[1][(size_t) i] = ring[1][(size_t) idx];
                    peak = std::max ({ peak, std::abs (ring[0][(size_t) idx]), std::abs (ring[1][(size_t) idx]) });
                    idx = (idx + 1) % preroll;
                }
                frames = pre;
            }
            startAbs = t - pre;
            skip = 0;
           #if OGI_TEST_HOOKS
            if (hooks.startOffset)
            {
                skip = 1;        // RC1n: the take starts one sample late
                startAbs += 1;
            }
           #endif

            // Length latched at the start sample (PD5).
            const bool tempo = load (pSyncMode, 0.0f) > 0.5f && tracker.isPlaying();
            const auto& an = tracker.anchor();
            std::int64_t tgt = takeLengthFrames (tempo, static_cast<int> (std::lround (load (pLenSync, 2.0f))), an.barLen, an.pps,
                                                 static_cast<double> (load (pLength, 4.0f)), fs);
            bool capped = true;
           #if OGI_TEST_HOOKS
            capped = ! hooks.capOff;
           #endif
            if (capped)
                tgt = std::min<std::int64_t> (tgt, capFrames);
            targetFrames = static_cast<int> (std::min<std::int64_t> (tgt, INT32_MAX));
            targetTel.store (targetFrames, std::memory_order_relaxed);
            slotTel.store (slot, std::memory_order_relaxed);
            state = RecState::Recording;
           #if OGI_TEST_HOOKS
            ++hooks.recStarts;
           #endif
        }

        /** Ends the take at absolute sample `at` (exclusive): commit or discard. */
        void stopTake (std::int64_t at) noexcept
        {
            stopAbs = at;
            if (! (peak >= kPresenceLin))
            {
                toIdle (RecReason::NoSignal);
                endArm (RecEnding::NoSignal);
               #if OGI_TEST_HOOKS
                ++hooks.recDiscards;
               #endif
                return;
            }
            if (frames < minFrames)
            {
                toIdle (RecReason::TooShort);
                endArm (RecEnding::TooShort);   // the timer writes only while the parameter is still On
               #if OGI_TEST_HOOKS
                ++hooks.recDiscards;
               #endif
                return;
            }
            commit.slot     = slot;
            commit.frames   = frames;
            commit.rate     = fs;
            commit.channels = channels;
            commit.startAbs = startAbs;
            state  = RecState::Committing;
            reason = RecReason::Committed;
            endArm (RecEnding::Committed);   // before `ready`: the poll that sees the take also sees its ending
            ready.store (frames, std::memory_order_release);
           #if OGI_TEST_HOOKS
            ++hooks.recCommits;
           #endif
        }

        void toIdle (RecReason r) noexcept
        {
            state = RecState::Idle;
            reason = r;
            nnAt = -1;
            ringCount = ringPos = 0;
        }

        /** D66: this arm is over and the parameter may still be On. Audio
            thread, or prepare() with the callback stopped. One relaxed store. */
        void endArm (RecEnding e) noexcept
        {
           #if OGI_TEST_HOOKS
            if (hooks.skipEndGen == static_cast<int> (e))
                return;
            ++hooks.armEnds;
           #else
            juce::ignoreUnused (e);
           #endif
            endGen.store (armGen.load (std::memory_order_relaxed), std::memory_order_relaxed);
        }

        void publish() noexcept
        {
            packed.store (static_cast<std::uint32_t> (state) | (static_cast<std::uint32_t> (reason) << 8)
                              | (heldArm ? (1u << 16) : 0u),
                          std::memory_order_relaxed);
        }

        static void scPeakPush (std::atomic<float>& acc, float v) noexcept
        {
            float cur = acc.load (std::memory_order_relaxed);
            while (v > cur && ! acc.compare_exchange_weak (cur, v, std::memory_order_relaxed))
            {
            }
        }

        // parameters (raw atomics)
        std::atomic<float>* pArm = nullptr;
        std::atomic<float>* pTarget = nullptr;
        std::atomic<float>* pTrigger = nullptr;
        std::atomic<float>* pThreshold = nullptr;
        std::atomic<float>* pLength = nullptr;
        std::atomic<float>* pLenSync = nullptr;
        std::atomic<float>* pGain = nullptr;
        std::atomic<float>* pSyncMode = nullptr;

        // audio-thread state
        double       fs = 44100.0;
        RecState     state = RecState::Idle;
        RecReason    reason = RecReason::None;
        RecTrigger   trigger = RecTrigger::Manual;
        bool         prevArm = false, heldArm = false;
        std::int64_t armAt = -1, startAbs = -1, stopAbs = -1, nnAt = -1, lastSample = 0;
        int          slot = 0, channels = 2, frames = 0, targetFrames = 0, minFrames = 1, skip = 0;
        float        peak = 0.0f, thrLin = 0.0316f;
        ExactRamp    gain;
        int          rampCells = 1;

        // buffers (prepare-allocated; grow-only)
        juce::CriticalSection captureLock;            // never taken on the audio thread
        std::unique_ptr<float[]> cap[2];
        int capacity = 0, capFrames = 0;
        std::unique_ptr<float[]> ring[2];
        int ringCap = 0, preroll = 1, ringCount = 0, ringPos = 0;

        // hand-off
        RecCommit commit;
        std::atomic<int> ready { 0 };
        std::atomic<bool> wavFailedFlag { false };

        // D66 auto-clear generations (relaxed; written on the audio thread and
        // in prepare(), read by the processor's message-thread timer)
        std::atomic<std::uint32_t> armGen { 0 }, endGen { 0 };

        // telemetry
        std::atomic<std::uint32_t> packed { 0 };
        std::atomic<int> framesTel { 0 }, targetTel { 0 }, slotTel { 0 };
        std::atomic<float> scPeakL { 0.0f }, scPeakR { 0.0f };   // PLAN-3.3 PD5 (replaces scPeak)
    };
} // namespace ogi
