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
// Phase 2.6 (PLAN-2.6 T5 / T8 / T9):
//   * Every slot carries its persisted state (source / name / fileSize /
//     contentHash / embed / trimmed + the FLAC-24 blob) in SlotInfo + the
//     record, written ONLY under the lock, atomically with the publish.
//   * Record-in commit (requestCommit): hash → float32 WAV into the Recordings
//     folder (BEFORE publish) → tape → publish (source rec:<path>) → FLAC-24
//     encode → blob cached for that generation. A WAV failure still publishes.
//   * Restore (restoreSlots, host thread): writes the records at once (a save
//     right after re-emits the same bytes), then queues one resolve job per
//     changed slot: path + size → embedded blob → Recordings filename + hash
//     search (bounded) → factory:<id> → missing (the clear sentinel; the
//     reference is kept). An identical-source restore is a no-op (PD12).
//   * Relink (requestRelink) / embed toggle (setSlotEmbed): message thread.
//   * The Recordings directory resolves lazily (never in a constructor).
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

        // 2.6 (PLAN-2.6 T9): the persisted reference + resolve flags.
        juce::String  source;              // factory:<id> | file:<abs> | rec:<abs> | empty
        juce::int64   fileSize       = 0;  // bytes of the file at path (0 = unknown)
        bool          hasHash        = false;
        std::uint64_t contentHash    = 0;  // GiHash contentHash of the pristine
        bool          embed          = false;   // D36: recordings default on, files opt-in, factory never
        bool          hasBlob        = false;   // a FLAC-24 blob is cached / carried
        bool          tooLarge       = false;   // the encode exceeded 16 MiB (not embedded)
        bool          missing        = false;   // no rung resolved: empty slot, reference kept
        bool          hashMismatch   = false;   // rung 1 loaded a file whose hash differs
        bool          relinkMismatch = false;   // a relink loaded a file with another hash
        bool          relocated      = false;   // rung 3 found the file elsewhere (path rewritten)
        bool          wavFailed      = false;   // the commit could not write its WAV
        int           resolvedBy     = 0;       // 0 none / direct, 1 path, 2 blob, 3 search, 4 factory, 5 missing
    };

    /** One <Slot> of the state (PLAN-2.6 PD21), parsed or emitted. */
    struct SavedSlot
    {
        int           index    = -1;
        juce::String  source, name;
        juce::int64   fileSize = 0;
        bool          hasHash  = false;
        std::uint64_t hash     = 0;
        bool          embed    = false;
        bool          trimmed  = false;
        juce::String  blob;               // base64 FLAC-24 (RFC 4648), may be empty
    };

   #if OGI_TEST_HOOKS
    /** PLAN-2.6 PD23 loader switches + counters (worker / message / host
        threads: atomics). */
    struct LoaderTestHooks
    {
        std::atomic<bool> reencodeOnLoad { false };   // ST3n: every resolve re-encodes at quality 8
        std::atomic<bool> searchUnbounded { false };  // RS8n: no 1024-entry / 16-hash bounds
        std::atomic<bool> floatWriterPath { false };  // ST2n / HS2: FLAC through writeFromAudioSampleBuffer
        std::atomic<bool> failWav { false };          // CM6: the WAV write fails
        std::atomic<std::uint64_t> fsTouches { 0 }, dirEntriesScanned { 0 }, candidatesHashed { 0 }, encodes { 0 },
                                   blobsCarried { 0 }, commitsPublished { 0 }, resolves { 0 };
        std::atomic<std::uint64_t> resolveRung[6] {};
        void resetCounters() noexcept
        {
            for (auto* a : { &fsTouches, &dirEntriesScanned, &candidatesHashed, &encodes, &blobsCarried, &commitsPublished, &resolves })
                a->store (0);
            for (auto& r : resolveRung)
                r.store (0);
        }
    };
   #endif

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

        static constexpr juce::int64 kEmbedMaxBytes    = 16777216;   // PD4: FLAC bytes (pre-base64)
        static constexpr int         kBlobMaxChars     = 22369624;   // 16 MiB · 4/3, a multiple of 4
        static constexpr int         kSearchMaxEntries = 1024;       // PD13 rung 3
        static constexpr int         kSearchMaxHashes  = 16;

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
        /** PLAN-2.4 PD6: Reset = an off-thread rebuild of the slot's working copy
            from its pristine at the session rate (slot −1 = all four). Bypasses
            the "already at this rate" early-out; deduped on a reset in flight.
            The engine takes it as a same-generation rebuild: the 20 ms
            dual-read crossfade, after its record head parks. */
        void requestReset (int slot);

        // Any non-audio thread.
        SlotInfo           getSlotInfo (int slot) const;
        std::vector<float> getOverview (int slot) const;

        //======================================================================
        // 2.6 (PLAN-2.6 T5 / T8 / T9)

        /** PD22: the Recordings folder (default userMusic/Ouaricon/
            O-GranulatorInstrument/Recordings, resolved lazily). */
        void       setRecordingsDirectory (const juce::File& dir);
        juce::File getRecordingsDirectory() const;

        /** Off-audio (the recorder poll): a captured take → worker commit job. */
        void requestCommit (int slot, std::shared_ptr<Pristine> take);
        /** Calling thread: the same commit body (harness commitSync). */
        bool commitSync (int slot, std::shared_ptr<Pristine> take, juce::String& error);

        /** Host thread (setStateInformation): PD21 — records written at once,
            resolve jobs queued; never decodes inline. */
        void restoreSlots (const std::vector<SavedSlot>& slots);
        /** Host thread (getStateInformation): the non-empty sources, copied
            under the lock; never encodes. */
        std::vector<SavedSlot> getSavedSlots() const;

        /** Message thread: load `file` into the slot as a relink (PD14). */
        void requestRelink (int slot, const juce::File& file);
        /** Message thread: D36 embed toggle (factory ignored). On → encode if
            no blob; off → the blob is dropped. */
        void setSlotEmbed (int slot, bool on);

        /** Harness: waits (sleeping) until the worker has no job, ≤ ms. */
        bool waitForIdle (int ms) const;

        /** The decoded hash of the slot's pristine (0 + false when empty). */
        bool getPristineHash (int slot, std::uint64_t& out) const;
        std::shared_ptr<const Pristine> getPristine (int slot) const;
        /** The cached / carried blob (base64), empty when none. */
        juce::String getBlob (int slot) const;

        // Static pieces (worker; also the harness rows).
        static bool decodeReader (juce::AudioFormatReader& reader, const juce::String& name, double fsSession,
                                  Pristine& out, juce::String& error, const CancelFn& cancelled, int maxFrames = -1);
        static bool decodeMemoryFlac (const void* data, std::size_t size, const juce::String& name, double fsSession,
                                      Pristine& out, juce::String& error);
        /** FLAC-24 of the pristine at its native rate (PD4: quality 5, Q codes
            through the int path). floatPath = the JUCE float writer (control). */
        static bool encodeFlac24 (const Pristine& p, juce::MemoryBlock& out, int quality = 5, bool floatPath = false);

        /** Set once by the processor before any job: a commit published
            (worker thread; must be lock-free on the callee side). */
        std::function<void (int slot, bool wavFailed)> onCommitted;

       #if OGI_TEST_HOOKS
        LoaderTestHooks hooks;
       #endif

        /** Calling thread: decode + build + publish. False + error on failure. */
        bool loadSync (int slot, const juce::File& file, juce::String& error);
        bool loadSync (int slot, const juce::AudioBuffer<float>& pcm, double sourceRate,
                       const juce::String& name, juce::String& error);
        /** Calling thread (the loadSync pattern; deterministic harness rows):
            build the slot's tape from its pristine at the session rate as a
            same-generation rebuild and publish it. False + error when the slot
            holds nothing. */
        bool resetSync (int slot, juce::String& error);

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
            bool                            resetInFlight = false;   // PLAN-2.4 PD6: a Reset job is queued / running
            juce::String                    blob;                    // 2.6: base64 FLAC-24 (carried or cached)
            std::uint64_t                   blobGen = 0;             // generation the blob belongs to
        };

        /** The persisted state a finished job installs together with its tape
            (under the lock, in one step with the publish). */
        struct NewState
        {
            juce::String  source, name;
            juce::int64   fileSize = 0;
            std::uint64_t hash = 0;
            bool          embed = false, keepBlob = false;
            juce::String  blob;   // installed when keepBlob
            bool hashMismatch = false, relinkMismatch = false, relocated = false, wavFailed = false;
            int  resolvedBy = 0;
        };

        void timerCallback() override;

        // 2.6 job bodies (worker, or the calling thread for the *Sync forms).
        enum class Kind { Load, Rebuild, Commit, Resolve, Relink, Encode };
        bool runLoadFile (int slot, std::uint64_t gen, const juce::File& file, double fs, const CancelFn& cancel,
                          const juce::ThreadPoolJob* job, juce::String& error);
        bool runCommit (int slot, std::uint64_t gen, std::shared_ptr<Pristine> take, double fs, const CancelFn& cancel,
                        const juce::ThreadPoolJob* job, juce::String& error);
        void runResolve (int slot, std::uint64_t gen, const SavedSlot& want, double fs, const CancelFn& cancel,
                         const juce::ThreadPoolJob* job);
        bool runRelink (int slot, std::uint64_t gen, const juce::File& file, double fs, const CancelFn& cancel,
                        const juce::ThreadPoolJob* job, juce::String& error);
        void runEncode (int slot, std::uint64_t gen, std::shared_ptr<const Pristine> src, int quality);
        /** Builds + publishes a decoded pristine with its new state; then the
            embed encode when wanted (after the publish, generation-checked). */
        bool buildAndPublish (int slot, std::uint64_t gen, std::shared_ptr<Pristine> src, double fs, NewState ns,
                              const CancelFn& cancel, const juce::ThreadPoolJob* job, juce::String& error);
        void markMissing (int slot, std::uint64_t gen, const juce::ThreadPoolJob* job);
        bool decodeFileCounted (const juce::File& f, double fs, Pristine& out, juce::String& error,
                                const CancelFn& cancel, int maxFrames = -1);
        bool hashFileHead (const juce::File& f, std::uint64_t& out);
        juce::File recordingsDirResolved() const;
        std::uint64_t bumpGenerationLocked (int slot);
        void enqueueLocked (int slot, Kind kind, std::uint64_t gen, const juce::File& file,
                            std::shared_ptr<const Pristine> src, const SavedSlot* want);

        // All *Locked functions expect `lock` held.
        void cancelActiveJobLocked (SlotRecord& r);
        void startRebuildLocked (int slot, double rate);
        void queueRebuildAsyncLocked (int slot, double rate);

        bool isSuperseded (int slot, std::uint64_t generation) const noexcept;

        /** Publishes a finished build if its generation is still current. */
        void publishResult (int slot, std::uint64_t generation, std::unique_ptr<SlotTape> tape,
                            std::shared_ptr<const Pristine> pristine, std::vector<float> overview,
                            bool rebuild, const juce::ThreadPoolJob* job, const NewState* ns);
        void jobFailed (int slot, std::uint64_t generation, const juce::String& error, bool cancelled,
                        const juce::ThreadPoolJob* job);

        juce::AudioFormatManager& formats;
        SlotHandoff& handoff;

        juce::CriticalSection lock;                       // never taken on the audio thread
        std::array<SlotRecord, kTapeSlots> records;
        juce::File recordingsDir;                         // guarded by lock (empty = the lazy default)
        std::array<std::atomic<std::uint64_t>, kTapeSlots> generationNow;   // lock-free supersession check
        double sessionRate = 44100.0;                     // guarded by lock
        std::atomic<bool> shuttingDown { false };
        std::shared_ptr<std::atomic<bool>> alive;         // guards callAsync lambdas

        juce::ThreadPool pool;

        JUCE_DECLARE_NON_COPYABLE (SlotLoader)
    };
} // namespace ogi
