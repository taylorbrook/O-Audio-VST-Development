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
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_events/juce_events.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>
#include "GiConstants.h"
#include "SlotTape.h"
#include "SlotHandoff.h"

// ============================================================================
// Slot loader (ARCHITECTURE §5, CONTEXT-2.1 Slot loader, RESEARCH-2.1 §7,
// PLAN-2.1 T7). Everything here is OFF the audio thread.
//
//   * 1-worker juce::ThreadPool. Jobs are keyed (slot, source, generation): a
//     newer request bumps the slot generation and removeJob (old, true, 0)s
//     the previous job; a job polls shouldExit() and its generation between
//     decode / resample / build chunks (every 64 k frames) and a superseded
//     result is dropped.
//   * Pristine kept at its native rate (mono stays mono). Trim at
//     min (30 s · fsIn, 2.88 M native frames, 2.88 M session frames ↔ native);
//     `trimmed` is set by whichever limit binds. A source shorter than 10 ms is
//     refused (error).
//   * Resample native → session with the offline Kaiser sinc (KaiserResampler.h);
//     mono → L = R at build. The tape is built through MipTape::writeRun
//     sequentially (the 2.4 record head reuses it), then flushTail().
//   * Overview: one (min, max) pair per 512 tape frames, both channels.
//   * SlotInfo lives under a CriticalSection that the audio thread NEVER takes;
//     prepareToPlay publishes from a host thread, so every SlotInfo write locks.
//   * SR change: setSessionRate (prepareToPlay) queues a rebuild from pristine
//     via MessageManager::callAsync, deduped by (slot, rate) on the message
//     thread. The rebuilt tape is published with rebuild = true (the engine
//     takes it with the 20 ms dual-read crossfade). A load that completes at a
//     stale rate is published (the reader corrects with tapeRate / fs) and
//     re-queues a rebuild.
//   * A juce::Timer reaper (100 ms) calls SlotHandoff::sweep (the producer-side
//     sweep runs inside every publish).
//
// Message-thread API shared with Phase 3.2 (drop, file browser):
//   requestSlotLoad (slot, File) / getSlotInfo (slot).
// loadSync (both forms) runs the same job body on the CALLING thread and
// publishes through the same pending exchange (render harness).
// ============================================================================
namespace ogi
{
    /** Per-slot status for the UI (a copy, taken under the loader lock). */
    struct SlotInfo
    {
        juce::String  name;                // display name (file name / given name)
        int           frames       = 0;    // tape frames at tapeRate
        double        nativeRate   = 0.0;  // source sample rate
        int           nativeFrames = 0;    // pristine frames (after trim)
        double        tapeRate     = 0.0;  // rate of the last published tape
        bool          trimmed      = false;
        bool          loaded       = false;
        bool          loading      = false;
        juce::String  lastError;
        std::uint64_t generation   = 0;
    };

    class SlotLoader : private juce::Timer
    {
    public:
        /** Pristine source at its native rate (mono stays mono). */
        struct Pristine
        {
            juce::AudioBuffer<float> pcm;
            double       rate    = 0.0;
            juce::String name;
            bool         trimmed = false;
        };

        using CancelFn = std::function<bool()>;

        SlotLoader (juce::AudioFormatManager& formats, SlotHandoff& handoff);
        ~SlotLoader() override;

        //======================================================================
        // Host thread (prepareToPlay). Queues SR rebuilds (callAsync, deduped).
        void   setSessionRate (double fs);
        double getSessionRate() const;

        // Message thread.
        void requestSlotLoad (int slot, const juce::File& file);
        void requestRebuild (int slot, double rate);

        // Any non-audio thread.
        SlotInfo           getSlotInfo (int slot) const;
        std::vector<float> getOverview (int slot) const;

        /** Calling thread: decode + build + publish. False + error on failure. */
        bool loadSync (int slot, const juce::File& file, juce::String& error);
        bool loadSync (int slot, const juce::AudioBuffer<float>& pcm, double sourceRate,
                       const juce::String& name, juce::String& error);

        //======================================================================
        // The job body pieces (static; also used by the harness rows L1 / L2).

        /** Native-frame cap: min (30 s · fsIn, 2.88 M, 2.88 M · fsIn / fsSession). */
        static std::int64_t nativeFrameCap (double fsIn, double fsSession) noexcept;

        static bool decodeFile (juce::AudioFormatManager& formats, const juce::File& file, double fsSession,
                                Pristine& out, juce::String& error, const CancelFn& cancelled);

        static bool pristineFromPcm (const juce::AudioBuffer<float>& pcm, double sourceRate, const juce::String& name,
                                     double fsSession, Pristine& out, juce::String& error);

        /** Resample + MipTape build + overview. nullptr + error on failure / cancel. */
        static std::unique_ptr<SlotTape> buildTape (const Pristine& source, double fsSession,
                                                    std::vector<float>& overview, const CancelFn& cancelled,
                                                    juce::String& error);

    private:
        class Job;
        friend class Job;

        struct SlotRecord
        {
            SlotInfo                        info;
            std::shared_ptr<const Pristine> pristine;
            std::vector<float>              overview;
            juce::ThreadPoolJob*            activeJob   = nullptr;   // owned by the pool
            double                          rebuildRate = 0.0;       // in-flight rebuild target (0 = none)
        };

        void timerCallback() override;

        // All *Locked functions expect `lock` held.
        void cancelActiveJobLocked (SlotRecord& r);
        void startRebuildLocked (int slot, double rate);
        void queueRebuildAsyncLocked (int slot, double rate);

        bool isSuperseded (int slot, std::uint64_t generation) const noexcept;

        /** Publishes a finished build if its generation is still current. */
        void publishResult (int slot, std::uint64_t generation, std::unique_ptr<SlotTape> tape,
                            std::shared_ptr<const Pristine> pristine, std::vector<float> overview,
                            bool rebuild, const juce::ThreadPoolJob* job);
        void jobFailed (int slot, std::uint64_t generation, const juce::String& error, bool cancelled,
                        const juce::ThreadPoolJob* job);

        juce::AudioFormatManager& formats;
        SlotHandoff& handoff;

        juce::CriticalSection lock;                       // never taken on the audio thread
        std::array<SlotRecord, kTapeSlots> records;
        std::array<std::atomic<std::uint64_t>, kTapeSlots> generationNow;   // lock-free supersession check
        double sessionRate = 44100.0;                     // guarded by lock
        std::atomic<bool> shuttingDown { false };
        std::shared_ptr<std::atomic<bool>> alive;         // guards callAsync lambdas

        juce::ThreadPool pool;

        JUCE_DECLARE_NON_COPYABLE (SlotLoader)
    };
} // namespace ogi
