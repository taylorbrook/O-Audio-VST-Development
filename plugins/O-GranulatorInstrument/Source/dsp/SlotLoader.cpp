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
#include "SlotLoader.h"
#include "KaiserResampler.h"

#include <algorithm>
#include <cmath>

namespace ogi
{
namespace
{
    constexpr int kDecodeChunk = 65536;    // frames per reader call (cancel poll)
    constexpr int kBuildChunk  = 4096;     // frames per writeRun
    constexpr int kCancelEvery = 65536;    // build frames between cancel polls
    constexpr int kReaperMs    = 100;

    bool validSlot (int slot) noexcept { return slot >= 0 && slot < kTapeSlots; }

    int minSourceFrames (double fsIn) noexcept
    {
        return std::max (1, static_cast<int> (std::ceil (kMinSourceMs * 0.001 * fsIn)));
    }

    void scrubNonFinite (juce::AudioBuffer<float>& b) noexcept
    {
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
        {
            float* d = b.getWritePointer (ch);
            for (int i = 0; i < b.getNumSamples(); ++i)
                if (! std::isfinite (d[i]))
                    d[i] = 0.0f;
        }
    }
}

//==============================================================================
class SlotLoader::Job final : public juce::ThreadPoolJob
{
public:
    Job (SlotLoader& o, int s, std::uint64_t gen, const juce::File& f,
         std::shared_ptr<const Pristine> src, bool isRebuild, double targetRate)
        : juce::ThreadPoolJob ("OGI slot load"),
          owner (o), slot (s), generation (gen), file (f), source (std::move (src)),
          rebuild (isRebuild), rate (targetRate)
    {
    }

    JobStatus runJob() override
    {
        const CancelFn cancelFn = [this]
        {
            return shouldExit() || owner.shuttingDown.load (std::memory_order_acquire)
                || owner.isSuperseded (slot, generation);
        };

        juce::String error;
        std::shared_ptr<const Pristine> src = source;
        if (src == nullptr)
        {
            auto decoded = std::make_shared<Pristine>();
            if (! decodeFile (owner.formats, file, rate, *decoded, error, cancelFn))
            {
                owner.jobFailed (slot, generation, error, cancelFn(), this);
                return jobHasFinished;
            }
            src = std::move (decoded);
        }

        std::vector<float> overview;
        auto tape = buildTape (*src, rate, overview, cancelFn, error);
        if (tape == nullptr)
        {
            owner.jobFailed (slot, generation, error, cancelFn(), this);
            return jobHasFinished;
        }

        tape->slot       = slot;
        tape->generation = generation;
        tape->rebuild    = rebuild;
        owner.publishResult (slot, generation, std::move (tape), std::move (src), std::move (overview), rebuild, this);
        return jobHasFinished;
    }

private:
    SlotLoader& owner;
    const int slot;
    const std::uint64_t generation;
    const juce::File file;
    std::shared_ptr<const Pristine> source;
    const bool rebuild;
    const double rate;
};

//==============================================================================
SlotLoader::SlotLoader (juce::AudioFormatManager& fm, SlotHandoff& h)
    : formats (fm),
      handoff (h),
      alive (std::make_shared<std::atomic<bool>> (true)),
      pool (juce::ThreadPoolOptions{}.withThreadName ("OGI loader").withNumberOfThreads (1))
{
    for (auto& g : generationNow)
        g.store (0, std::memory_order_relaxed);

    // The reaper Timer needs a message loop; consoles without one still get
    // the producer-side sweep inside every publish.
    if (juce::MessageManager::getInstanceWithoutCreating() != nullptr)
        startTimer (kReaperMs);
}

SlotLoader::~SlotLoader()
{
    shuttingDown.store (true, std::memory_order_release);
    alive->store (false, std::memory_order_release);
    stopTimer();
    pool.removeAllJobs (true, 10000);

    const juce::ScopedLock sl (lock);
    for (auto& r : records)
        r.activeJob = nullptr;
}

void SlotLoader::timerCallback()
{
    handoff.sweep();
}

//==============================================================================
std::int64_t SlotLoader::nativeFrameCap (double fsIn, double fsSession) noexcept
{
    if (! (fsIn > 0.0))
        return 0;
    const double bySeconds = std::floor (kSlotMaxSeconds * fsIn);
    const double byNative  = static_cast<double> (kSlotMaxFrames);
    const double bySession = fsSession > 0.0 ? std::floor (static_cast<double> (kSlotMaxFrames) * fsIn / fsSession) : byNative;
    return static_cast<std::int64_t> (std::min (bySeconds, std::min (byNative, bySession)));
}

bool SlotLoader::decodeFile (juce::AudioFormatManager& fm, const juce::File& file, double fsSession,
                             Pristine& out, juce::String& error, const CancelFn& cancelled)
{
    if (! file.existsAsFile())
    {
        error = "file not found: " + file.getFullPathName();
        return false;
    }

    std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (file));
    if (reader == nullptr)
    {
        error = "unsupported or unreadable audio file: " + file.getFileName();
        return false;
    }

    const double fsIn = reader->sampleRate;
    const int numCh = std::min (2, static_cast<int> (reader->numChannels));
    if (! (fsIn > 0.0) || numCh < 1)
    {
        error = "invalid audio format: " + file.getFileName();
        return false;
    }

    const std::int64_t total = reader->lengthInSamples;
    const std::int64_t cap   = nativeFrameCap (fsIn, fsSession);
    const std::int64_t take  = std::min (total, cap);
    if (take < static_cast<std::int64_t> (minSourceFrames (fsIn)))
    {
        error = "source shorter than 10 ms: " + file.getFileName();
        return false;
    }

    const int frames = static_cast<int> (take);
    out.pcm.setSize (numCh, frames, false, true, false);
    out.pcm.clear();
    for (int pos = 0; pos < frames; pos += kDecodeChunk)
    {
        const int n = std::min (kDecodeChunk, frames - pos);
        float* dst[2] = { out.pcm.getWritePointer (0, pos), numCh > 1 ? out.pcm.getWritePointer (1, pos) : nullptr };
        if (! reader->read (dst, numCh, static_cast<juce::int64> (pos), n))
        {
            error = "read error: " + file.getFileName();
            return false;
        }
        if (cancelled && cancelled())
        {
            error = "cancelled";
            return false;
        }
    }
    scrubNonFinite (out.pcm);

    out.rate    = fsIn;
    out.name    = file.getFileName();
    out.trimmed = total > cap;
    return true;
}

bool SlotLoader::pristineFromPcm (const juce::AudioBuffer<float>& pcm, double sourceRate, const juce::String& name,
                                  double fsSession, Pristine& out, juce::String& error)
{
    const int numCh = std::min (2, pcm.getNumChannels());
    if (! (sourceRate > 0.0) || numCh < 1)
    {
        error = "invalid source buffer (rate or channels)";
        return false;
    }

    const std::int64_t total = pcm.getNumSamples();
    const std::int64_t cap   = nativeFrameCap (sourceRate, fsSession);
    const std::int64_t take  = std::min (total, cap);
    if (take < static_cast<std::int64_t> (minSourceFrames (sourceRate)))
    {
        error = "source shorter than 10 ms";
        return false;
    }

    const int frames = static_cast<int> (take);
    out.pcm.setSize (numCh, frames, false, true, false);
    for (int ch = 0; ch < numCh; ++ch)
        out.pcm.copyFrom (ch, 0, pcm, ch, 0, frames);
    scrubNonFinite (out.pcm);

    out.rate    = sourceRate;
    out.name    = name;
    out.trimmed = total > cap;
    return true;
}

std::unique_ptr<SlotTape> SlotLoader::buildTape (const Pristine& source, double fsSession,
                                                 std::vector<float>& overview, const CancelFn& cancelled,
                                                 juce::String& error)
{
    const int numCh = source.pcm.getNumChannels();
    const int nIn   = source.pcm.getNumSamples();
    if (numCh < 1 || nIn < 1 || ! (fsSession > 0.0))
    {
        error = "nothing to build";
        return {};
    }

    std::vector<float> left, right;
    if (! kaiser::resample (source.pcm.getReadPointer (0), nIn, source.rate, fsSession, left, cancelled))
    {
        error = "cancelled";
        return {};
    }
    if (numCh > 1)
    {
        if (! kaiser::resample (source.pcm.getReadPointer (1), nIn, source.rate, fsSession, right, cancelled))
        {
            error = "cancelled";
            return {};
        }
    }
    else
    {
        right = left;   // mono → L = R at tape build
    }

    const int frames = static_cast<int> (std::min<std::size_t> (std::min (left.size(), right.size()),
                                                                static_cast<std::size_t> (kSlotMaxFrames)));
    if (frames < 1)
    {
        error = "empty tape";
        return {};
    }

    auto tape = std::make_unique<SlotTape>();
    tape->tape.prepare (frames);
    for (int pos = 0; pos < frames; pos += kBuildChunk)
    {
        const int n = std::min (kBuildChunk, frames - pos);
        tape->tape.writeRun (pos, n, left.data() + pos, right.data() + pos);
        if ((pos % kCancelEvery) == 0 && cancelled && cancelled())
        {
            error = "cancelled";
            return {};
        }
    }
    tape->tape.flushTail();
    tape->rate   = fsSession;
    tape->frames = frames;

    const int bins = (frames + kOverviewBin - 1) / kOverviewBin;
    overview.assign (static_cast<std::size_t> (2 * bins), 0.0f);
    for (int b = 0; b < bins; ++b)
    {
        const int i0 = b * kOverviewBin;
        const int i1 = std::min (frames, i0 + kOverviewBin);
        float lo = left[(size_t) i0], hi = left[(size_t) i0];
        for (int i = i0; i < i1; ++i)
        {
            lo = std::min (lo, std::min (left[(size_t) i], right[(size_t) i]));
            hi = std::max (hi, std::max (left[(size_t) i], right[(size_t) i]));
        }
        overview[(size_t) (2 * b)]     = lo;
        overview[(size_t) (2 * b + 1)] = hi;
    }
    return tape;
}

//==============================================================================
bool SlotLoader::isSuperseded (int slot, std::uint64_t generation) const noexcept
{
    return ! validSlot (slot) || generationNow[(size_t) slot].load (std::memory_order_acquire) != generation;
}

void SlotLoader::cancelActiveJobLocked (SlotRecord& r)
{
    if (r.activeJob != nullptr)
    {
        // Queued → removed (and deleted by the pool); running → signalled, no wait.
        pool.removeJob (r.activeJob, true, 0);
        r.activeJob = nullptr;
    }
    r.rebuildRate = 0.0;
    r.resetInFlight = false;
}

void SlotLoader::requestSlotLoad (int slot, const juce::File& file)
{
    if (! validSlot (slot) || shuttingDown.load (std::memory_order_acquire))
        return;

    const juce::ScopedLock sl (lock);
    auto& r = records[(size_t) slot];
    cancelActiveJobLocked (r);
    const std::uint64_t gen = ++r.info.generation;
    generationNow[(size_t) slot].store (gen, std::memory_order_release);
    r.info.loading = true;
    r.info.lastError.clear();

    auto* job = new Job (*this, slot, gen, file, nullptr, false, sessionRate);
    r.activeJob = job;
    pool.addJob (job, true);
}

void SlotLoader::setSessionRate (double fs)
{
    if (! (fs > 0.0))
        return;

    const juce::ScopedLock sl (lock);
    sessionRate = fs;
    for (int s = 0; s < kTapeSlots; ++s)
    {
        const auto& r = records[(size_t) s];
        if (r.info.loaded && r.pristine != nullptr && ! r.info.loading && ! kaiser::ratesEqual (r.info.tapeRate, fs))
            queueRebuildAsyncLocked (s, fs);
    }
}

double SlotLoader::getSessionRate() const
{
    const juce::ScopedLock sl (lock);
    return sessionRate;
}

void SlotLoader::queueRebuildAsyncLocked (int slot, double rate)
{
    if (juce::MessageManager::getInstanceWithoutCreating() == nullptr)
    {
        startRebuildLocked (slot, rate);   // no message loop (console): rebuild directly
        return;
    }
    auto flag = alive;
    juce::MessageManager::callAsync ([flag, this, slot, rate]
    {
        if (flag->load (std::memory_order_acquire))
            requestRebuild (slot, rate);
    });
}

void SlotLoader::requestRebuild (int slot, double rate)
{
    if (! validSlot (slot) || shuttingDown.load (std::memory_order_acquire))
        return;

    const juce::ScopedLock sl (lock);
    const auto& r = records[(size_t) slot];
    if (! kaiser::ratesEqual (rate, sessionRate))
        return;                                         // a stale request (the rate moved on)
    if (! r.info.loaded || r.pristine == nullptr || r.info.loading)
        return;                                         // the load in flight re-queues on completion
    if (kaiser::ratesEqual (r.info.tapeRate, rate))
        return;                                         // already at this rate
    if (r.activeJob != nullptr && kaiser::ratesEqual (r.rebuildRate, rate))
        return;                                         // dedupe (slot, rate) in flight
    startRebuildLocked (slot, rate);
}

void SlotLoader::requestReset (int slot)
{
    if (shuttingDown.load (std::memory_order_acquire))
        return;
    if (slot == -1)
    {
        for (int s = 0; s < kTapeSlots; ++s)
            requestReset (s);
        return;
    }
    if (! validSlot (slot))
        return;

    const juce::ScopedLock sl (lock);
    auto& r = records[(size_t) slot];
    if (! r.info.loaded || r.pristine == nullptr || r.info.loading)
        return;                                         // nothing to reset (a load in flight owns the slot)
    if (r.resetInFlight && r.activeJob != nullptr)
        return;                                         // dedupe: a Reset of this slot is already queued / running
    startRebuildLocked (slot, sessionRate);             // bypasses requestRebuild's "already at this rate" early-out
    r.resetInFlight = true;
}

bool SlotLoader::resetSync (int slot, juce::String& error)
{
    if (! validSlot (slot))
    {
        error = "slot out of range";
        return false;
    }

    std::shared_ptr<const Pristine> src;
    std::uint64_t gen = 0;
    double fs = 0.0;
    {
        const juce::ScopedLock sl (lock);
        auto& r = records[(size_t) slot];
        if (! r.info.loaded || r.pristine == nullptr)
        {
            error = "slot holds no source";
            return false;
        }
        cancelActiveJobLocked (r);
        src = r.pristine;
        gen = r.info.generation;
        fs  = sessionRate;
    }

    const CancelFn cancel = [this, slot, gen] { return isSuperseded (slot, gen); };
    std::vector<float> overview;
    auto tape = buildTape (*src, fs, overview, cancel, error);
    if (tape == nullptr)
        return false;
    tape->slot       = slot;
    tape->generation = gen;
    tape->rebuild    = true;
    publishResult (slot, gen, std::move (tape), std::move (src), std::move (overview), true, nullptr);
    return true;
}

void SlotLoader::startRebuildLocked (int slot, double rate)
{
    auto& r = records[(size_t) slot];
    if (r.pristine == nullptr)
        return;
    cancelActiveJobLocked (r);
    r.rebuildRate = rate;
    auto* job = new Job (*this, slot, r.info.generation, juce::File(), r.pristine, true, rate);
    r.activeJob = job;
    pool.addJob (job, true);
}

//==============================================================================
void SlotLoader::publishResult (int slot, std::uint64_t generation, std::unique_ptr<SlotTape> tape,
                                std::shared_ptr<const Pristine> pristine, std::vector<float> overview,
                                bool rebuild, const juce::ThreadPoolJob* job)
{
    const juce::ScopedLock sl (lock);
    auto& r = records[(size_t) slot];
    if (job != nullptr && r.activeJob == job)
    {
        r.activeJob = nullptr;
        r.rebuildRate = 0.0;
        r.resetInFlight = false;
    }
    if (generation != r.info.generation || tape == nullptr || shuttingDown.load (std::memory_order_acquire))
        return;   // superseded: the tape was never published and dies here

    r.info.name         = pristine != nullptr ? pristine->name : juce::String();
    r.info.frames       = tape->frames;
    r.info.tapeRate     = tape->rate;
    r.info.nativeRate   = pristine != nullptr ? pristine->rate : 0.0;
    r.info.nativeFrames = pristine != nullptr ? pristine->pcm.getNumSamples() : 0;
    r.info.trimmed      = pristine != nullptr && pristine->trimmed;
    r.info.loaded       = true;
    if (! rebuild)
        r.info.loading = false;
    r.info.lastError.clear();
    r.pristine = std::move (pristine);
    r.overview = std::move (overview);

    const double tapeRate = tape->rate;
    handoff.publish (slot, tape.release());

    if (! kaiser::ratesEqual (tapeRate, sessionRate))
        queueRebuildAsyncLocked (slot, sessionRate);    // built at a stale rate: re-rate
}

void SlotLoader::jobFailed (int slot, std::uint64_t generation, const juce::String& error, bool cancelled,
                            const juce::ThreadPoolJob* job)
{
    const juce::ScopedLock sl (lock);
    auto& r = records[(size_t) slot];
    bool wasRebuild = false;
    if (job != nullptr && r.activeJob == job)
    {
        r.activeJob = nullptr;
        wasRebuild = r.rebuildRate > 0.0;
        r.rebuildRate = 0.0;
        r.resetInFlight = false;
    }
    if (generation != r.info.generation || cancelled)
        return;   // a newer request owns the slot status
    if (! wasRebuild)
        r.info.loading = false;
    r.info.lastError = error;
}

//==============================================================================
SlotInfo SlotLoader::getSlotInfo (int slot) const
{
    if (! validSlot (slot))
        return {};
    const juce::ScopedLock sl (lock);
    return records[(size_t) slot].info;
}

std::vector<float> SlotLoader::getOverview (int slot) const
{
    if (! validSlot (slot))
        return {};
    const juce::ScopedLock sl (lock);
    return records[(size_t) slot].overview;
}

bool SlotLoader::loadSync (int slot, const juce::File& file, juce::String& error)
{
    if (! validSlot (slot))
    {
        error = "slot out of range";
        return false;
    }

    std::uint64_t gen = 0;
    double fs = 0.0;
    {
        const juce::ScopedLock sl (lock);
        auto& r = records[(size_t) slot];
        cancelActiveJobLocked (r);
        gen = ++r.info.generation;
        generationNow[(size_t) slot].store (gen, std::memory_order_release);
        r.info.loading = true;
        r.info.lastError.clear();
        fs = sessionRate;
    }

    const CancelFn cancel = [this, slot, gen] { return isSuperseded (slot, gen); };
    auto decoded = std::make_shared<Pristine>();
    if (! decodeFile (formats, file, fs, *decoded, error, cancel))
    {
        jobFailed (slot, gen, error, false, nullptr);
        return false;
    }

    std::vector<float> overview;
    auto tape = buildTape (*decoded, fs, overview, cancel, error);
    if (tape == nullptr)
    {
        jobFailed (slot, gen, error, false, nullptr);
        return false;
    }
    tape->slot = slot;
    tape->generation = gen;
    tape->rebuild = false;
    publishResult (slot, gen, std::move (tape), std::move (decoded), std::move (overview), false, nullptr);
    return true;
}

bool SlotLoader::loadSync (int slot, const juce::AudioBuffer<float>& pcm, double sourceRate,
                           const juce::String& name, juce::String& error)
{
    if (! validSlot (slot))
    {
        error = "slot out of range";
        return false;
    }

    std::uint64_t gen = 0;
    double fs = 0.0;
    {
        const juce::ScopedLock sl (lock);
        auto& r = records[(size_t) slot];
        cancelActiveJobLocked (r);
        gen = ++r.info.generation;
        generationNow[(size_t) slot].store (gen, std::memory_order_release);
        r.info.loading = true;
        r.info.lastError.clear();
        fs = sessionRate;
    }

    auto source = std::make_shared<Pristine>();
    if (! pristineFromPcm (pcm, sourceRate, name, fs, *source, error))
    {
        jobFailed (slot, gen, error, false, nullptr);
        return false;
    }

    const CancelFn cancel = [this, slot, gen] { return isSuperseded (slot, gen); };
    std::vector<float> overview;
    auto tape = buildTape (*source, fs, overview, cancel, error);
    if (tape == nullptr)
    {
        jobFailed (slot, gen, error, false, nullptr);
        return false;
    }
    tape->slot = slot;
    tape->generation = gen;
    tape->rebuild = false;
    publishResult (slot, gen, std::move (tape), std::move (source), std::move (overview), false, nullptr);
    return true;
}
} // namespace ogi
