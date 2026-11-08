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
#include <array>
#include <climits>
#include <cstdint>
#include <vector>

#include "BandlimitedReader.h"   // grain-engine 1.1.0
#include "CounterRng.h"
#include "GrainPool.h"
#include "GrainScheduler.h"

#include "GiConstants.h"
#include "forked/ControlGrid.h"
#include "forked/Grain.h"
#include "forked/OutputLaw.h"
#include "forked/WindowMorphBank.h"
#include "AmpEnv.h"
#include "GiRenderer.h"
#include "GiSpawner.h"
#include "MidiCursor.h"
#include "ScanPlayhead.h"
#include "SlotHandoff.h"
#include "SlotTape.h"
#include "VoiceAllocator.h"
#include "VoiceGrain.h"
#include "VoiceQuotas.h"

// ============================================================================
// Engine — Phase 2.1 (PLAN-2.1 T8, ARCHITECTURE Processing Chain).
//
// Absolute 32-sample grid pass engine. The host block is cut at every
// absolute multiple of kCell; each piece [a, b) runs:
//   1. grid tick (at a multiple of 32): latch GiGridParams, ramps, take the
//      pending tapes (kill fade / SR-rebuild crossfade), finish crossfades,
//      retire tapes no grain holds (288-slot scan), loop region, polyphony
//      shrink (engine events BEFORE the sample's host events), quotas
//      (water-fill + P1 thinning, rescheduleNext on a quota decrease), law
//      gains, scan re-anchor;
//   2. host MIDI at its clamped offsets (iterated in place; P5), splitting the
//      piece into segments;
//   3. per segment: amp envs (a[k][n]), schedulers (Free, one per counted
//      voice), onsets merged by (offset, serial) together with voice-end
//      points, rendered grain-major in sub-segments split at those points;
//   4. mix: Σ_k a_k · g_out,k · bus_k, × s1 level / pan / audible ramps ×
//      kVoiceGainDb (−6 dB, provisional) → wet safety 1.9953·tanh (x/1.9953)
//      → output_gain → out. Reverb absent (unity), feedback return zeroed (P8).
//
// Stream 1 reads slot 0. Every random draw keys on (seed, kGrainStream + 0,
// (serial << 24) | onsetIndex) — never the voice slot. The only 2.1 draw is
// s1_size_rand (P2).
//
// Threads: process() is the audio thread (no lock, no allocation; the caller
// holds ScopedNoDenormals). prepare() / bind() / the constructor run on a
// host / message thread while process() is not running.
// ============================================================================
namespace ogi
{
   #if OGI_TEST_HOOKS
    /** One spawned grain (row V3 / N1 per-serial logs). */
    struct GrainLogEntry
    {
        std::uint64_t serial      = 0;
        std::uint64_t onsetIndex  = 0;    // scheduler index since note-on
        std::int64_t  onsetSample = 0;    // absolute engine sample of the onset
        double        frac        = 0.0;  // onset fraction [0, 1)
        double        len         = 0.0;  // grain length, output samples
        double        startPos    = 0.0;  // tape frames (region-fit p0, before the frac advance)
        double        rate        = 0.0;  // tape frames per output sample
        int           voice       = 0;    // voice slot (NOT part of the identity)
        int           level       = 0;
        int           poolSlot    = 0;
    };

    /** Render-harness hooks. Plain fields written on the audio thread and read
        by the (single-threaded) harness between processBlock calls. Fixed-size
        storage only; the log is sized once in the Engine constructor. */
    struct EngineTestHooks
    {
        static constexpr int kLogCapacity    = 65536;
        static constexpr int kStolenCapacity = 256;

        // ── switches (set before rendering) ──────────────────────────────────
        bool thinningOff   = false;   // Q1b control: m = ∞ (no integer thinning)
        bool overQuotaOff  = false;   // Q2 control: acquire's 2·overQuota term zeroed
        bool periodThinOff = false;   // Q2 regime: spawn period ignores m (demand outruns the quota) but acquire keeps the finite m
        bool d25Off        = false;   // Q3 control: no over-cap convergence kill
        bool stealRampZero = false;   // V2 control: voice steal / poly shrink cut in 0 ms
        bool retireScan256 = false;   // H3 control: retire scan of the 256 live slots only
                                      //   (an early retire is COUNTED in earlyRetires and
                                      //    the tape is kept, so the control cannot UAF)
        bool seedOverrideOn = false;  // N1 / V3: replace the processor seed
        std::uint64_t seedOverride = 0;
        bool slotPermOn = false;      // V3: free-slot search order = slotPerm[0..19]
        int  slotPerm[kVoiceSlots] {};

        // ── voice readouts (per voice SLOT) ───────────────────────────────────
        double        voiceRate[kVoiceSlots]   {};   // rate the voice latches now (tick, note-on, spawn)
        int           voiceLevel[kVoiceSlots]  {};   // latchLevel (rate, 4) of that rate
        std::uint64_t voiceSerial[kVoiceSlots] {};
        int           voiceNote[kVoiceSlots]   {};
        int           voiceState[kVoiceSlots]  {};   // 0 free, 1 counted, 2 stolen

        // ── grain log (ring) ──────────────────────────────────────────────────
        std::vector<GrainLogEntry> log;              // kLogCapacity entries
        std::uint64_t logCount = 0;                  // total written (ring index = n % capacity)

        // ── counters ──────────────────────────────────────────────────────────
        std::uint64_t spawns = 0, r3Kills = 0, nativeSteals = 0, capKills = 0, swapKills = 0;
        std::uint64_t voiceSteals = 0, polyForceReleases = 0, gridTicks = 0;
        std::uint64_t stolenSerials[kStolenCapacity] {};   // ring of stolen note serials
        std::uint64_t stolenCount = 0;
        std::uint64_t tapesTaken = 0, xfadesStarted = 0, earlyRetires = 0, retiredByEngine = 0;

        // ── per-tick pool / quota state (written at every grid tick) ──────────
        int    liveNow = 0;                 // non-killing grains in the 256 live slots
        int    liveMin = INT_MAX, liveMax = 0;
        int    activeNow = 0;               // every sounding slot (live + killing + fade), 288
        int    liveVoice[kVoiceSlots] {};   // non-killing live grains per voice slot
        int    quotaM[kVoiceSlots]    {};   // P1 thinning integer (INT_MAX = uncapped)
        double quotaQ[kVoiceSlots]    {};
        double periodT[kVoiceSlots]   {};   // thinned onset period, samples
        double lambda = 0.0;
        bool   capped = false;

        // ── slots ─────────────────────────────────────────────────────────────
        double regionLo = 0.0, regionHi = 0.0;      // s1 loop region, tape frames
        bool   xfadeActive[kTapeSlots] {};
        double tapeRate[kTapeSlots]    {};
        int    retiringNow = 0;

        /** Zeroes counters, min / max trackers and the log index (not switches). */
        void resetCounters() noexcept
        {
            logCount = 0;
            spawns = r3Kills = nativeSteals = capKills = swapKills = 0;
            voiceSteals = polyForceReleases = gridTicks = 0;
            stolenCount = 0;
            tapesTaken = xfadesStarted = earlyRetires = retiredByEngine = 0;
            liveMin = INT_MAX;
            liveMax = 0;
        }

        /** The log in chronological order (copy; harness thread). */
        std::vector<GrainLogEntry> logInOrder() const
        {
            std::vector<GrainLogEntry> out;
            if (log.empty())
                return out;
            const std::uint64_t cap = static_cast<std::uint64_t> (log.size());
            const std::uint64_t n = logCount < cap ? logCount : cap;
            out.reserve (static_cast<std::size_t> (n));
            for (std::uint64_t i = logCount - n; i < logCount; ++i)
                out.push_back (log[static_cast<std::size_t> (i % cap)]);
            return out;
        }

        /** Identity permutation for slotPerm, then offset (V3 helper). */
        void setSlotOffset (int offset) noexcept
        {
            for (int i = 0; i < kVoiceSlots; ++i)
                slotPerm[i] = ((i + offset) % kVoiceSlots + kVoiceSlots) % kVoiceSlots;
            slotPermOn = true;
        }
    };
   #endif

    class Engine
    {
    public:
        explicit Engine (SlotHandoff& handoff);
        ~Engine();

        /** Message thread (processor constructor). */
        void bind (juce::AudioProcessorValueTreeState& apvts);

        /** Host thread (prepareToPlay); process() is not running. Allocates the
            feedback return; clears voices and pool; retires idle tapes. */
        void prepare (double sampleRate, int maxBlockSize);

        /** Audio thread. outL / outR must be CLEARED by the caller; numSamples
            may exceed the prepared block size (everything is 32-sample scratch). */
        void process (float* outL, float* outR, int numSamples, const juce::MidiBuffer& midi,
                      std::uint64_t seed) noexcept;

        double       getSampleRate() const noexcept     { return fs; }
        std::int64_t getSamplePosition() const noexcept { return absPos; }
        const SlotTape* getCurrentTape (int slot) const noexcept
        {
            return (slot >= 0 && slot < kTapeSlots) ? current[(size_t) slot] : nullptr;
        }

        /** Rate (tape frames per output sample) a voice on `note` latches with the
            current grid parameters and the current slot-0 tape (0 without a tape). */
        double rateForNote (int note) const noexcept;

       #if OGI_TEST_HOOKS
        EngineTestHooks hooks;
        const GiPool&    getPool() const noexcept           { return pool; }
        /** H3 stimulus: re-prepare the pool with a longer steal fade so a fade
            spare can outlive the 5 ms swap kills (clears the pool; call before
            any note). The shipping fade stays kPoolFadeMs. */
        void testSetPoolFadeMs (double ms) noexcept       { pool.prepare (fs, ms); }
        const GiSpawner& getSpawner() const noexcept        { return spawner; }
        const Voice&     getVoice (int k) const noexcept    { return voices[std::clamp (k, 0, kVoiceSlots - 1)]; }
        const GiGridParams& getGridParams() const noexcept  { return gp; }
        int              getRetiringCount() const noexcept  { return numRetiring; }
        const LoopRegion& getRegion() const noexcept        { return region; }
       #endif

    private:
        struct PendingOnset
        {
            int           voice  = 0;
            int           offset = 0;    // segment-relative
            double        frac   = 0.0;
            std::uint64_t index  = 0;
            std::uint64_t serial = 0;
        };

        struct PendingEnd
        {
            int           voice  = 0;
            int           offset = 0;    // segment-relative
            std::uint64_t serial = 0;
        };

        // grid
        void gridTick (std::int64_t t) noexcept;
        void takeTapes (std::int64_t t) noexcept;
        void finishXfades (std::int64_t t) noexcept;
        void retireScan() noexcept;
        void killGrainsOfTape (const SlotTape* tape) noexcept;
        void updateRegion() noexcept;
        void enforcePolyphony() noexcept;
        void updateQuotas() noexcept;
        void updateScan (std::int64_t t) noexcept;

        // events
        void applyEvent (const MidiEvent& e, std::int64_t t) noexcept;
        void noteOn (int channel, int note, float velocity01, std::int64_t t) noexcept;
        void noteOff (int channel, int note) noexcept;
        void allNotesOff() noexcept;
        void allSoundOff() noexcept;
        void startVoice (int k, int channel, int note, float velocity01, std::int64_t t) noexcept;
        void stealVoice (int k) noexcept;
        void forceReleaseVoice (int k) noexcept;
        void endVoice (int k) noexcept;
        bool endQuietestStolen() noexcept;

        // audio
        void runSegment (int c0, int c1, std::int64_t t0) noexcept;
        void renderGrains (int c0, int c1, std::int64_t t0) noexcept;
        void spawn (const PendingOnset& o, std::int64_t tAbs) noexcept;
        void mixPiece (int c0, int c1, float* outL, float* outR) noexcept;

        // helpers
        AmpParams ampParams() const noexcept;
        double voiceRate (const Voice& v, const SlotTape& tape) const noexcept;
        double uncappedPeriod() const noexcept;
        double lMaxSamples() const noexcept;
        double lenBarSamples() const noexcept;
        void   writeVoiceHooks (int k) noexcept;

        SlotHandoff& handoff;
        GiParamPointers ptrs;
        GiGridParams gp;
        WindowMorphBank morphBank;   // built once here (message thread); Hann-only law in 2.1

        double        fs = 44100.0;
        std::int64_t  absPos = 0;
        std::uint64_t seed = 0;
        float         bend = 0.0f;   // −1..1, channel pitch wheel (non-MPE; any channel)
        float         voiceGainLin = 1.0f;

        GiPool    pool;
        GiSpawner spawner;

        Voice voices[kVoiceSlots];
        std::uint64_t serialCounter = 0;

        std::array<SlotTape*, kTapeSlots> current {};
        SlotTape* retiring[kMaxRetiring] {};
        int numRetiring = 0;
        XfadeState xfade[kTapeSlots];

        float    envBuf[kVoiceSlots][kCell] {};
        VoiceBus bus;
        CellLerp lawGain[kVoiceSlots];
        float    lawTarget[kVoiceSlots] {};

        ExactRamp outGainRamp, levelRamp, panLRamp, panRRamp, audibleRamp;
        int rampCells20 = 1, rampCells10 = 1;

        QuotasN<kVoiceSlots> quotas;
        int    quotaM[kVoiceSlots] {};
        double periodT[kVoiceSlots] {};
        double prevDemand[kVoiceSlots] {};
        int    prevCap = -1;
        double prevLMax = -1.0, prevDensity = -1.0;
        int   prevThinOff = 0;
        bool   quotasDirty = true;

        ScanAnchor freeRun;
        LoopRegion region;

        std::vector<float> fbReturn;   // P8: allocated in prepare, zero-filled, never written in 2.1

        ouaricon::grain::Onset onsetBuf[kVoiceSlots][kOnsetCap] {};
        PendingOnset merged[kVoiceSlots * kOnsetCap] {};
        PendingEnd   ends[kVoiceSlots] {};

        JUCE_DECLARE_NON_COPYABLE (Engine)
    };
} // namespace ogi
