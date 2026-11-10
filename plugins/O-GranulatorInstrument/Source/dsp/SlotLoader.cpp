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
#include "GiFactory.h"
#include "GiHash.h"

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

    // 2.6 source schemes (ARCH File I/O and Sources).
    const juce::String kFile ("file:"), kRec ("rec:"), kFactory ("factory:");

    juce::String schemeOf (const juce::String& source)
    {
        for (const auto& p : { kFile, kRec, kFactory })
            if (source.startsWith (p))
                return p;
        return {};
    }

    /** The absolute path of a file: / rec: source (empty otherwise). */
    juce::String pathOf (const juce::String& source)
    {
        const auto sc = schemeOf (source);
        return (sc == kFile || sc == kRec) ? source.substring (sc.length()) : juce::String();
    }

    /** float32 WAV (the Record-in master copy). */
    bool writeWavFloat (const juce::File& f, const juce::AudioBuffer<float>& pcm, double rate)
    {
        auto fos = std::make_unique<juce::FileOutputStream> (f);
        if (! fos->openedOk())
            return false;
        std::unique_ptr<juce::OutputStream> os (std::move (fos));
        juce::WavAudioFormat wav;
        const auto opts = juce::AudioFormatWriterOptions{}
                              .withSampleRate (rate)
                              .withNumChannels (pcm.getNumChannels())
                              .withBitsPerSample (32)
                              .withSampleFormat (juce::AudioFormatWriterOptions::SampleFormat::floatingPoint);
        auto w = wav.createWriterFor (os, opts);
        if (w == nullptr)
            return false;
        const bool ok = w->writeFromAudioSampleBuffer (pcm, 0, pcm.getNumSamples());
        w.reset();
        return ok;
    }
}

//==============================================================================
class SlotLoader::Job final : public juce::ThreadPoolJob
{
public:
    Job (SlotLoader& o, int s, Kind k, std::uint64_t gen, const juce::File& f,
         std::shared_ptr<const Pristine> src, double targetRate, const SavedSlot* w = nullptr)
        : juce::ThreadPoolJob ("OGI slot load"),
          owner (o), slot (s), kind (k), generation (gen), file (f), source (std::move (src)), rate (targetRate)
    {
        if (w != nullptr)
            want = *w;
    }

    JobStatus runJob() override
    {
        const CancelFn cancelFn = [this]
        {
            return shouldExit() || owner.shuttingDown.load (std::memory_order_acquire)
                || owner.isSuperseded (slot, generation);
        };

        juce::String error;
        switch (kind)
        {
            case Kind::Load:
                if (! owner.runLoadFile (slot, generation, file, rate, cancelFn, this, error))
                    owner.jobFailed (slot, generation, error, cancelFn(), this);
                break;

            case Kind::Rebuild:
            {
                std::vector<float> overview;
                auto tape = buildTape (*source, rate, overview, cancelFn, error);
                if (tape == nullptr)
                {
                    owner.jobFailed (slot, generation, error, cancelFn(), this);
                    break;
                }
                tape->slot       = slot;
                tape->generation = generation;
                tape->rebuild    = true;
                owner.publishResult (slot, generation, std::move (tape), source, std::move (overview), true, this, nullptr);
                break;
            }

            case Kind::Commit:
                if (! owner.runCommit (slot, generation, std::const_pointer_cast<Pristine> (source), rate, cancelFn, this, error))
                    owner.jobFailed (slot, generation, error, cancelFn(), this);
                break;

            case Kind::Resolve:
                owner.runResolve (slot, generation, want, rate, cancelFn, this);
                break;

            case Kind::Relink:
                if (! owner.runRelink (slot, generation, file, rate, cancelFn, this, error))
                    owner.jobFailed (slot, generation, error, cancelFn(), this);
                break;

            case Kind::Encode:
                owner.runEncode (slot, generation, source, 5);
                break;
        }
        return jobHasFinished;
    }

private:
    SlotLoader& owner;
    const int slot;
    const Kind kind;
    const std::uint64_t generation;
    const juce::File file;
    std::shared_ptr<const Pristine> source;
    const double rate;
    SavedSlot want;
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
    return decodeReader (*reader, file.getFileName(), fsSession, out, error, cancelled);
}

bool SlotLoader::decodeReader (juce::AudioFormatReader& reader, const juce::String& name, double fsSession,
                               Pristine& out, juce::String& error, const CancelFn& cancelled, int maxFrames)
{
    const double fsIn = reader.sampleRate;
    const int numCh = std::min (2, static_cast<int> (reader.numChannels));
    if (! (fsIn > 0.0) || numCh < 1)
    {
        error = "invalid audio format: " + name;
        return false;
    }

    const std::int64_t total = reader.lengthInSamples;
    const std::int64_t cap   = nativeFrameCap (fsIn, fsSession);
    std::int64_t take  = std::min (total, cap);
    if (take < static_cast<std::int64_t> (minSourceFrames (fsIn)))
    {
        error = "source shorter than 10 ms: " + name;
        return false;
    }
    if (maxFrames > 0)
        take = std::min<std::int64_t> (take, maxFrames);   // the hash head (rung 3)

    const int frames = static_cast<int> (take);
    out.pcm.setSize (numCh, frames, false, true, false);
    out.pcm.clear();
    for (int pos = 0; pos < frames; pos += kDecodeChunk)
    {
        const int n = std::min (kDecodeChunk, frames - pos);
        float* dst[2] = { out.pcm.getWritePointer (0, pos), numCh > 1 ? out.pcm.getWritePointer (1, pos) : nullptr };
        if (! reader.read (dst, numCh, static_cast<juce::int64> (pos), n))
        {
            error = "read error: " + name;
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
    out.name    = name;
    out.trimmed = total > cap;
    return true;
}

bool SlotLoader::decodeMemoryFlac (const void* data, std::size_t size, const juce::String& name, double fsSession,
                                   Pristine& out, juce::String& error)
{
    if (data == nullptr || size == 0)
    {
        error = "empty FLAC data";
        return false;
    }
    juce::FlacAudioFormat flac;
    std::unique_ptr<juce::AudioFormatReader> reader (flac.createReaderFor (new juce::MemoryInputStream (data, size, false), true));
    if (reader == nullptr)
    {
        error = "unreadable FLAC data: " + name;
        return false;
    }
    return decodeReader (*reader, name, fsSession, out, error, {});
}

bool SlotLoader::encodeFlac24 (const Pristine& p, juce::MemoryBlock& out, int quality, bool floatPath)
{
    const int C = std::min (2, p.pcm.getNumChannels());
    const int n = p.pcm.getNumSamples();
    if (C < 1 || n < 1 || ! (p.rate > 0.0))
        return false;

    juce::MemoryBlock mb;
    {
        std::unique_ptr<juce::OutputStream> os (new juce::MemoryOutputStream (mb, false));
        juce::FlacAudioFormat flac;
        const auto opts = juce::AudioFormatWriterOptions{}
                              .withSampleRate (p.rate)
                              .withNumChannels (C)
                              .withBitsPerSample (24)
                              .withQualityOptionIndex (quality);   // PD4: 5 explicitly (index 0 is silently also 5)
        auto w = flac.createWriterFor (os, opts);
        if (w == nullptr)
            return false;

        if (floatPath)
        {
            // HS2 / ST2n control: JUCE's float path (floor of round32 / 256, RS2).
            if (! w->writeFromAudioSampleBuffer (p.pcm, 0, n))
                return false;
        }
        else
        {
            // PD3 / PD4: our quantiser, fed to the writer as 32-bit-left-justified
            // ints (the FLAC writer shifts right by 8 for 24-bit).
            constexpr int kChunk = 4096;
            std::vector<int> q[2];
            q[0].resize (kChunk);
            q[1].resize (kChunk);
            for (int pos = 0; pos < n; pos += kChunk)
            {
                const int m = std::min (kChunk, n - pos);
                for (int c = 0; c < C; ++c)
                {
                    const float* x = p.pcm.getReadPointer (c, pos);
                    for (int i = 0; i < m; ++i)
                        q[c][(size_t) i] = static_cast<int> (gihash::quantise (x[i])) * 256;
                }
                const int* ptrs[3] = { q[0].data(), C > 1 ? q[1].data() : nullptr, nullptr };
                if (! w->write (ptrs, m))
                    return false;
            }
        }
        w.reset();   // finishes the stream and patches STREAMINFO (seek-back on the memory stream)
    }
    out = std::move (mb);
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
    const std::uint64_t gen = bumpGenerationLocked (slot);
    r.info.loading = true;
    r.info.lastError.clear();

    auto* job = new Job (*this, slot, Kind::Load, gen, file, nullptr, sessionRate);
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
    publishResult (slot, gen, std::move (tape), std::move (src), std::move (overview), true, nullptr, nullptr);
    return true;
}

void SlotLoader::startRebuildLocked (int slot, double rate)
{
    auto& r = records[(size_t) slot];
    if (r.pristine == nullptr)
        return;
    cancelActiveJobLocked (r);
    r.rebuildRate = rate;
    auto* job = new Job (*this, slot, Kind::Rebuild, r.info.generation, juce::File(), r.pristine, rate);
    r.activeJob = job;
    pool.addJob (job, true);
}

//==============================================================================
void SlotLoader::publishResult (int slot, std::uint64_t generation, std::unique_ptr<SlotTape> tape,
                                std::shared_ptr<const Pristine> pristine, std::vector<float> overview,
                                bool rebuild, const juce::ThreadPoolJob* job, const NewState* ns)
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

    // PLAN-2.6 T9: the persisted state lands in the same locked step as the
    // publish (a save never sees a source without its tape, or the reverse).
    if (ns != nullptr)
    {
        r.info.source         = ns->source;
        if (ns->name.isNotEmpty())
            r.info.name       = ns->name;
        r.info.fileSize       = ns->fileSize;
        r.info.hasHash        = true;
        r.info.contentHash    = ns->hash;
        r.info.embed          = ns->embed && ! ns->source.startsWith (kFactory);
        r.info.tooLarge       = false;
        r.info.missing        = false;
        r.info.hashMismatch   = ns->hashMismatch;
        r.info.relinkMismatch = ns->relinkMismatch;
        r.info.relocated      = ns->relocated;
        r.info.wavFailed      = ns->wavFailed;
        r.info.resolvedBy     = ns->resolvedBy;
        if (ns->keepBlob && ns->blob.isNotEmpty())
        {
            r.blob    = ns->blob;
            r.blobGen = generation;
        }
        else
        {
            r.blob.clear();
            r.blobGen = 0;
        }
        r.info.hasBlob = r.blob.isNotEmpty();
    }

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
        gen = bumpGenerationLocked (slot);
        r.info.loading = true;
        r.info.lastError.clear();
        fs = sessionRate;
    }

    const CancelFn cancel = [this, slot, gen] { return isSuperseded (slot, gen); };
    if (! runLoadFile (slot, gen, file, fs, cancel, nullptr, error))
    {
        jobFailed (slot, gen, error, false, nullptr);
        return false;
    }
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
        gen = bumpGenerationLocked (slot);
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

    // A harness buffer has no source string (never persisted, never embedded).
    NewState ns;
    ns.name = name;
    ns.hash = gihash::contentHash (source->pcm);
    const CancelFn cancel = [this, slot, gen] { return isSuperseded (slot, gen); };
    if (! buildAndPublish (slot, gen, source, fs, ns, cancel, nullptr, error))
    {
        jobFailed (slot, gen, error, false, nullptr);
        return false;
    }
    return true;
}

//==============================================================================
// Phase 2.6 (PLAN-2.6 T5 / T8 / T9)
std::uint64_t SlotLoader::bumpGenerationLocked (int slot)
{
    auto& r = records[(size_t) slot];
    const std::uint64_t gen = ++r.info.generation;
    generationNow[(size_t) slot].store (gen, std::memory_order_release);
    return gen;
}

void SlotLoader::enqueueLocked (int slot, Kind kind, std::uint64_t gen, const juce::File& file,
                                std::shared_ptr<const Pristine> src, const SavedSlot* want)
{
    auto& r = records[(size_t) slot];
    auto* job = new Job (*this, slot, kind, gen, file, std::move (src), sessionRate, want);
    if (kind != Kind::Encode)
        r.activeJob = job;   // an encode never owns the slot (a load / rebuild never cancels it)
    pool.addJob (job, true);
}

void SlotLoader::setRecordingsDirectory (const juce::File& dir)
{
    const juce::ScopedLock sl (lock);
    recordingsDir = dir;
}

juce::File SlotLoader::getRecordingsDirectory() const
{
    return recordingsDirResolved();
}

juce::File SlotLoader::recordingsDirResolved() const
{
    {
        const juce::ScopedLock sl (lock);
        if (recordingsDir != juce::File())
            return recordingsDir;
    }
    // PD22: resolved lazily (first commit / rung 3), never in a constructor.
    return juce::File::getSpecialLocation (juce::File::userMusicDirectory)
               .getChildFile ("Ouaricon").getChildFile ("O-GranulatorInstrument").getChildFile ("Recordings");
}

bool SlotLoader::decodeFileCounted (const juce::File& f, double fs, Pristine& out, juce::String& error,
                                    const CancelFn& cancel, int maxFrames)
{
   #if OGI_TEST_HOOKS
    hooks.fsTouches.fetch_add (1);
   #endif
    if (! f.existsAsFile())
    {
        error = "file not found: " + f.getFullPathName();
        return false;
    }
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (f));
    if (reader == nullptr)
    {
        error = "unsupported or unreadable audio file: " + f.getFileName();
        return false;
    }
    return decodeReader (*reader, f.getFileName(), fs, out, error, cancel, maxFrames);
}

bool SlotLoader::hashFileHead (const juce::File& f, std::uint64_t& out)
{
    Pristine head;
    juce::String error;
    if (! decodeFileCounted (f, 0.0, head, error, {}, gihash::kHashFrames))
        return false;
    out = gihash::contentHash (head.pcm);
    return true;
}

bool SlotLoader::buildAndPublish (int slot, std::uint64_t gen, std::shared_ptr<Pristine> src, double fs, NewState ns,
                                  const CancelFn& cancel, const juce::ThreadPoolJob* job, juce::String& error)
{
    std::vector<float> overview;
    auto tape = buildTape (*src, fs, overview, cancel, error);
    if (tape == nullptr)
        return false;
    tape->slot       = slot;
    tape->generation = gen;
    tape->rebuild    = false;

    bool encode = ns.embed && ! ns.source.startsWith (kFactory) && ! (ns.keepBlob && ns.blob.isNotEmpty());
   #if OGI_TEST_HOOKS
    if (ns.keepBlob && ns.blob.isNotEmpty())
        hooks.blobsCarried.fetch_add (1);
    if (hooks.reencodeOnLoad.load() && ns.resolvedBy > 0 && ns.embed)
        encode = true;
   #endif
    std::shared_ptr<const Pristine> keep = src;
    publishResult (slot, gen, std::move (tape), std::move (src), std::move (overview), false, job, &ns);

    if (encode)
    {
        int quality = 5;
       #if OGI_TEST_HOOKS
        if (hooks.reencodeOnLoad.load() && ns.resolvedBy > 0)
            quality = 8;
       #endif
        runEncode (slot, gen, keep, quality);
    }
    return true;
}

void SlotLoader::runEncode (int slot, std::uint64_t gen, std::shared_ptr<const Pristine> src, int quality)
{
    if (src == nullptr || isSuperseded (slot, gen) || shuttingDown.load (std::memory_order_acquire))
        return;
    bool floatPath = false;
   #if OGI_TEST_HOOKS
    floatPath = hooks.floatWriterPath.load();
    hooks.encodes.fetch_add (1);
   #endif
    juce::MemoryBlock mb;
    const bool ok = encodeFlac24 (*src, mb, quality, floatPath);
    const bool tooLarge = ok && static_cast<juce::int64> (mb.getSize()) > kEmbedMaxBytes;
    const juce::String b64 = (ok && ! tooLarge) ? juce::Base64::toBase64 (mb.getData(), mb.getSize()) : juce::String();

    const juce::ScopedLock sl (lock);
    auto& r = records[(size_t) slot];
    if (gen != r.info.generation || ! r.info.embed || r.pristine != src)
        return;   // superseded, embed switched off, or another pristine since
    r.info.tooLarge = tooLarge;
    r.blob    = b64;
    r.blobGen = b64.isNotEmpty() ? gen : 0;
    r.info.hasBlob = b64.isNotEmpty();
}

bool SlotLoader::runLoadFile (int slot, std::uint64_t gen, const juce::File& file, double fs, const CancelFn& cancel,
                              const juce::ThreadPoolJob* job, juce::String& error)
{
    auto decoded = std::make_shared<Pristine>();
    if (! decodeFileCounted (file, fs, *decoded, error, cancel))
        return false;
    NewState ns;
    ns.source   = kFile + file.getFullPathName();
    ns.name     = file.getFileName();
    ns.fileSize = file.getSize();
    ns.hash     = gihash::contentHash (decoded->pcm);
    ns.embed    = false;   // D36: user files are opt-in
    return buildAndPublish (slot, gen, decoded, fs, ns, cancel, job, error);
}

void SlotLoader::requestCommit (int slot, std::shared_ptr<Pristine> take)
{
    if (! validSlot (slot) || take == nullptr || shuttingDown.load (std::memory_order_acquire))
        return;
    const juce::ScopedLock sl (lock);
    auto& r = records[(size_t) slot];
    cancelActiveJobLocked (r);
    const std::uint64_t gen = bumpGenerationLocked (slot);
    r.info.loading = true;
    r.info.lastError.clear();
    enqueueLocked (slot, Kind::Commit, gen, {}, std::move (take), nullptr);
}

bool SlotLoader::commitSync (int slot, std::shared_ptr<Pristine> take, juce::String& error)
{
    if (! validSlot (slot) || take == nullptr)
    {
        error = "bad commit";
        return false;
    }
    std::uint64_t gen = 0;
    double fs = 0.0;
    {
        const juce::ScopedLock sl (lock);
        auto& r = records[(size_t) slot];
        cancelActiveJobLocked (r);
        gen = bumpGenerationLocked (slot);
        r.info.loading = true;
        r.info.lastError.clear();
        fs = sessionRate;
    }
    const CancelFn cancel = [this, slot, gen] { return isSuperseded (slot, gen); };
    if (! runCommit (slot, gen, std::move (take), fs, cancel, nullptr, error))
    {
        jobFailed (slot, gen, error, false, nullptr);
        return false;
    }
    return true;
}

bool SlotLoader::runCommit (int slot, std::uint64_t gen, std::shared_ptr<Pristine> take, double fs, const CancelFn& cancel,
                            const juce::ThreadPoolJob* job, juce::String& error)
{
    if (take == nullptr || take->pcm.getNumSamples() < 1)
    {
        error = "empty take";
        return false;
    }
    scrubNonFinite (take->pcm);
    take->trimmed = false;

    // PD8: hash → WAV (BEFORE publish) → tape → publish → FLAC-24 encode.
    NewState ns;
    ns.hash  = gihash::contentHash (take->pcm);
    ns.embed = true;   // D36: recordings embed by default

    const auto dir = recordingsDirResolved();
    const auto stamp = juce::Time::getCurrentTime().formatted ("%Y%m%d-%H%M%S");
    const auto base = "GI-Rec-" + stamp + "-S" + juce::String (slot + 1) + ".wav";
    bool wavOk = false;
    juce::File f = dir.getChildFile (base);
   #if OGI_TEST_HOOKS
    hooks.fsTouches.fetch_add (1);
    const bool failWav = hooks.failWav.load();
   #else
    const bool failWav = false;
   #endif
    if (! failWav && dir.createDirectory())
    {
        f = f.getNonexistentSibling (false);
        wavOk = writeWavFloat (f, take->pcm, take->rate);
        if (! wavOk)
            f.deleteFile();
    }
    ns.source    = kRec + f.getFullPathName();   // the intended path even when the write failed
    ns.name      = f.getFileName();
    ns.fileSize  = wavOk ? f.getSize() : 0;
    ns.wavFailed = ! wavOk;
    take->name   = ns.name;

    if (! buildAndPublish (slot, gen, take, fs, ns, cancel, job, error))
        return false;
   #if OGI_TEST_HOOKS
    hooks.commitsPublished.fetch_add (1);
   #endif
    if (onCommitted)
        onCommitted (slot, ns.wavFailed);
    return true;
}

void SlotLoader::markMissing (int slot, std::uint64_t gen, const juce::ThreadPoolJob* job)
{
    const juce::ScopedLock sl (lock);
    auto& r = records[(size_t) slot];
    if (job != nullptr && r.activeJob == job)
    {
        r.activeJob = nullptr;
        r.rebuildRate = 0.0;
        r.resetInFlight = false;
    }
    if (gen != r.info.generation || shuttingDown.load (std::memory_order_acquire))
        return;
    // PD11: a loaded slot is emptied through the sentinel; the reference (source,
    // name, size, hash, embed, blob) is KEPT so a save re-emits it unchanged.
    if (r.info.loaded)
        handoff.publishClear (slot);
    r.info.loaded = false;
    r.info.loading = false;
    r.info.frames = 0;
    r.info.tapeRate = 0.0;
    r.info.nativeFrames = 0;
    r.info.missing = true;
    r.info.resolvedBy = 5;
    r.pristine.reset();
    r.overview.clear();
}

void SlotLoader::runResolve (int slot, std::uint64_t gen, const SavedSlot& want, double fs, const CancelFn& cancel,
                             const juce::ThreadPoolJob* job)
{
   #if OGI_TEST_HOOKS
    hooks.resolves.fetch_add (1);
    auto rung = [this] (int k) { hooks.resolveRung[k].fetch_add (1); };
   #else
    auto rung = [] (int) {};
   #endif
    const auto scheme = schemeOf (want.source);
    const auto path   = pathOf (want.source);
    juce::String error;

    NewState base;
    base.source   = want.source;
    base.name     = want.name;
    base.fileSize = want.fileSize;
    base.embed    = want.embed && scheme != kFactory;

    // Rung 1: the path exists AND its size matches (PD13: a hash mismatch still loads, flagged).
    if (path.isNotEmpty() && juce::File::isAbsolutePath (path))
    {
        const juce::File f (path);
       #if OGI_TEST_HOOKS
        hooks.fsTouches.fetch_add (1);
       #endif
        if (f.existsAsFile() && (want.fileSize <= 0 || f.getSize() == want.fileSize))
        {
            auto p = std::make_shared<Pristine>();
            if (decodeFileCounted (f, fs, *p, error, cancel))
            {
                NewState ns = base;
                ns.hash = gihash::contentHash (p->pcm);
                ns.hashMismatch = want.hasHash && ns.hash != want.hash;
                ns.keepBlob = ! ns.hashMismatch && want.blob.isNotEmpty();   // the carry rule
                ns.blob = want.blob;
                ns.resolvedBy = 1;
                if (ns.name.isEmpty())
                    ns.name = f.getFileName();
                rung (1);
                if (! buildAndPublish (slot, gen, p, fs, ns, cancel, job, error) && ! cancel())
                    jobFailed (slot, gen, error, false, job);
                return;
            }
        }
    }
    if (cancel())
        return;

    // Rung 2: the embedded blob, verified against the stored hash.
    if (want.blob.isNotEmpty())
    {
        juce::MemoryOutputStream mo;
        if (juce::Base64::convertFromBase64 (mo, want.blob))
        {
            auto p = std::make_shared<Pristine>();
            if (decodeMemoryFlac (mo.getData(), mo.getDataSize(), want.name, fs, *p, error))
            {
                const auto h = gihash::contentHash (p->pcm);
                if (! want.hasHash || h == want.hash)
                {
                    NewState ns = base;
                    ns.hash = h;
                    ns.keepBlob = true;
                    ns.blob = want.blob;
                    ns.resolvedBy = 2;
                    rung (2);
                    if (! buildAndPublish (slot, gen, p, fs, ns, cancel, job, error) && ! cancel())
                        jobFailed (slot, gen, error, false, job);
                    return;
                }
            }
        }
    }
    if (cancel())
        return;

    // Rung 3: the Recordings folder — exact filename, then a bounded flat scan
    // (≤ 1024 entries, size prefilter, ≤ 16 hashed). A hit rewrites the path.
    if (want.hasHash && path.isNotEmpty())
    {
        const auto dir = recordingsDirResolved();
        const auto fileName = juce::File::createFileWithoutCheckingPath (path).getFileName();
        bool unbounded = false;
       #if OGI_TEST_HOOKS
        unbounded = hooks.searchUnbounded.load();
        hooks.fsTouches.fetch_add (1);
       #endif
        juce::File found;
        int hashed = 0;
        if (dir.isDirectory())
        {
            const auto exact = dir.getChildFile (fileName);
            std::uint64_t h = 0;
            if (exact.existsAsFile() && (want.fileSize <= 0 || exact.getSize() == want.fileSize))
            {
                ++hashed;
               #if OGI_TEST_HOOKS
                hooks.candidatesHashed.fetch_add (1);
               #endif
                if (hashFileHead (exact, h) && h == want.hash)
                    found = exact;
            }
            if (found == juce::File())
            {
                int entries = 0;
                for (const auto& e : juce::RangedDirectoryIterator (dir, false, "*.wav", juce::File::findFiles))
                {
                    if (cancel())
                        return;
                    if (! unbounded && entries >= kSearchMaxEntries)
                        break;
                    ++entries;
                   #if OGI_TEST_HOOKS
                    hooks.dirEntriesScanned.fetch_add (1);
                   #endif
                    const auto f = e.getFile();
                    if (f == exact || (want.fileSize > 0 && e.getFileSize() != want.fileSize))
                        continue;
                    if (! unbounded && hashed >= kSearchMaxHashes)
                        break;
                    ++hashed;
                   #if OGI_TEST_HOOKS
                    hooks.candidatesHashed.fetch_add (1);
                   #endif
                    if (hashFileHead (f, h) && h == want.hash)
                    {
                        found = f;
                        break;
                    }
                }
            }
        }
        if (found != juce::File())
        {
            auto p = std::make_shared<Pristine>();
            if (decodeFileCounted (found, fs, *p, error, cancel))
            {
                NewState ns = base;
                ns.hash = gihash::contentHash (p->pcm);
                ns.source = (scheme.isNotEmpty() ? scheme : kFile) + found.getFullPathName();
                ns.name = found.getFileName();
                ns.fileSize = found.getSize();
                ns.relocated = found.getFullPathName() != path;
                ns.keepBlob = want.blob.isNotEmpty() && ns.hash == want.hash;
                ns.blob = want.blob;
                ns.resolvedBy = 3;
                rung (3);
                if (! buildAndPublish (slot, gen, p, fs, ns, cancel, job, error) && ! cancel())
                    jobFailed (slot, gen, error, false, job);
                return;
            }
        }
    }
    if (cancel())
        return;

    // Rung 4: factory:<id> — decoded from memory, no disk access (D33).
    if (scheme == kFactory)
    {
        FactoryEntry e;
        if (findFactory (want.source.substring (kFactory.length()), e))
        {
            auto p = std::make_shared<Pristine>();
            if (decodeMemoryFlac (e.data, e.size, juce::String (e.id), fs, *p, error))
            {
                NewState ns = base;
                ns.hash = gihash::contentHash (p->pcm);
                ns.embed = false;   // factory never embeds
                ns.resolvedBy = 4;
                if (ns.name.isEmpty())
                    ns.name = juce::String (e.id);
                rung (4);
                if (! buildAndPublish (slot, gen, p, fs, ns, cancel, job, error) && ! cancel())
                    jobFailed (slot, gen, error, false, job);
                return;
            }
        }
    }
    if (cancel())
        return;

    // Rung 5: missing (the plugin stays stable; the reference is kept).
    rung (5);
    markMissing (slot, gen, job);
}

void SlotLoader::restoreSlots (const std::vector<SavedSlot>& slots)
{
    if (shuttingDown.load (std::memory_order_acquire))
        return;
    const juce::ScopedLock sl (lock);
    for (int s = 0; s < kTapeSlots; ++s)
    {
        const SavedSlot* w = nullptr;
        for (const auto& x : slots)
            if (x.index == s && x.source.isNotEmpty())
                w = &x;
        auto& r = records[(size_t) s];

        if (w == nullptr)
        {
            // An empty source clears a SOURCED slot through the publish path
            // (PD11). A slot without a source string (a harness buffer) is
            // never persisted, so a restore leaves it alone.
            if (r.info.source.isEmpty())
                continue;
            cancelActiveJobLocked (r);
            bumpGenerationLocked (s);
            if (r.info.loaded)
                handoff.publishClear (s);
            const auto gen = r.info.generation;
            r.info = SlotInfo {};
            r.info.generation = gen;
            r.pristine.reset();
            r.overview.clear();
            r.blob.clear();
            r.blobGen = 0;
            continue;
        }

        const bool factory = w->source.startsWith (kFactory);
        // PD12: an identical-source restore is a no-op (no job; the working
        // copy is not reset by a host re-send).
        if (r.info.source == w->source && r.info.loaded && ! r.info.loading && ! r.info.missing
            && r.info.hasHash && w->hasHash && r.info.contentHash == w->hash)
        {
            r.info.embed = w->embed && ! factory;
            if (r.info.embed && r.blob.isEmpty() && w->blob.isNotEmpty())
            {
                r.blob = w->blob;
                r.blobGen = r.info.generation;
                r.info.hasBlob = true;
            }
            else if (! r.info.embed)
            {
                r.blob.clear();
                r.info.hasBlob = false;
            }
            continue;
        }

        cancelActiveJobLocked (r);
        const auto gen = bumpGenerationLocked (s);
        // PD21: the records hold the restored reference at once (a save before
        // the resolve finishes re-emits it), then the resolve job is queued.
        r.info.source         = w->source;
        r.info.name           = w->name;
        r.info.fileSize       = w->fileSize;
        r.info.hasHash        = w->hasHash;
        r.info.contentHash    = w->hash;
        r.info.embed          = w->embed && ! factory;
        r.info.trimmed        = w->trimmed;
        r.info.missing        = false;
        r.info.tooLarge       = false;
        r.info.hashMismatch   = false;
        r.info.relinkMismatch = false;
        r.info.relocated      = false;
        r.info.wavFailed      = false;
        r.info.resolvedBy     = 0;
        r.info.loading        = true;
        r.info.lastError.clear();
        r.blob    = w->blob;
        r.blobGen = 0;
        r.info.hasBlob = r.blob.isNotEmpty();
        enqueueLocked (s, Kind::Resolve, gen, {}, nullptr, w);
    }
}

std::vector<SavedSlot> SlotLoader::getSavedSlots() const
{
    std::vector<SavedSlot> out;
    const juce::ScopedLock sl (lock);
    for (int s = 0; s < kTapeSlots; ++s)
    {
        const auto& r = records[(size_t) s];
        if (r.info.source.isEmpty())
            continue;
        SavedSlot x;
        x.index    = s;
        x.source   = r.info.source;
        x.name     = r.info.name;
        x.fileSize = r.info.fileSize;
        x.hasHash  = r.info.hasHash;
        x.hash     = r.info.contentHash;
        x.embed    = r.info.embed;
        x.trimmed  = r.info.trimmed;
        if (r.info.embed)
            x.blob = r.blob;   // carried / cached; never encoded here (a pending encode → no blob)
        out.push_back (std::move (x));
    }
    return out;
}

void SlotLoader::requestRelink (int slot, const juce::File& file)
{
    if (! validSlot (slot) || shuttingDown.load (std::memory_order_acquire))
        return;
    const juce::ScopedLock sl (lock);
    auto& r = records[(size_t) slot];
    cancelActiveJobLocked (r);
    const auto gen = bumpGenerationLocked (slot);
    r.info.loading = true;
    r.info.lastError.clear();
    enqueueLocked (slot, Kind::Relink, gen, file, nullptr, nullptr);
}

bool SlotLoader::runRelink (int slot, std::uint64_t gen, const juce::File& file, double fs, const CancelFn& cancel,
                            const juce::ThreadPoolJob* job, juce::String& error)
{
    auto p = std::make_shared<Pristine>();
    if (! decodeFileCounted (file, fs, *p, error, cancel))
        return false;

    juce::String oldSource, oldBlob;
    bool oldHasHash = false, oldEmbed = false;
    std::uint64_t oldHash = 0;
    {
        const juce::ScopedLock sl (lock);
        const auto& r = records[(size_t) slot];
        oldSource  = r.info.source;
        oldHasHash = r.info.hasHash;
        oldHash    = r.info.contentHash;
        oldEmbed   = r.info.embed;
        oldBlob    = r.blob;
    }

    NewState ns;
    ns.hash     = gihash::contentHash (p->pcm);
    ns.name     = file.getFileName();
    ns.fileSize = file.getSize();
    const auto scheme = schemeOf (oldSource);
    if (oldHasHash && ns.hash == oldHash)
    {
        // PD14 match: the scheme, embed and the carried blob are kept.
        ns.source   = ((scheme == kFile || scheme == kRec) ? scheme : kFile) + file.getFullPathName();
        ns.embed    = oldEmbed;
        ns.keepBlob = oldBlob.isNotEmpty();
        ns.blob     = oldBlob;
    }
    else
    {
        // Mismatch: still loads, as a plain file, embed off, flagged.
        ns.source         = kFile + file.getFullPathName();
        ns.embed          = false;
        ns.relinkMismatch = true;
    }
    return buildAndPublish (slot, gen, p, fs, ns, cancel, job, error);
}

void SlotLoader::setSlotEmbed (int slot, bool on)
{
    if (! validSlot (slot) || shuttingDown.load (std::memory_order_acquire))
        return;
    const juce::ScopedLock sl (lock);
    auto& r = records[(size_t) slot];
    if (r.info.source.isEmpty() || r.info.source.startsWith (kFactory))
        return;   // factory never embeds
    if (on)
    {
        r.info.embed = true;
        if (r.blob.isEmpty() && r.pristine != nullptr && ! r.info.tooLarge)
            enqueueLocked (slot, Kind::Encode, r.info.generation, {}, r.pristine, nullptr);
    }
    else
    {
        r.info.embed = false;
        r.blob.clear();
        r.blobGen = 0;
        r.info.hasBlob = false;
    }
}

bool SlotLoader::waitForIdle (int ms) const
{
    const auto end = juce::Time::getMillisecondCounterHiRes() + (double) ms;
    while (pool.getNumJobs() > 0)
    {
        if (juce::Time::getMillisecondCounterHiRes() > end)
            return false;
        juce::Thread::sleep (1);
    }
    return true;
}

bool SlotLoader::getPristineHash (int slot, std::uint64_t& out) const
{
    if (! validSlot (slot))
        return false;
    const juce::ScopedLock sl (lock);
    const auto& r = records[(size_t) slot];
    if (r.pristine == nullptr || ! r.info.hasHash)
        return false;
    out = r.info.contentHash;
    return true;
}

std::shared_ptr<const SlotLoader::Pristine> SlotLoader::getPristine (int slot) const
{
    if (! validSlot (slot))
        return {};
    const juce::ScopedLock sl (lock);
    return records[(size_t) slot].pristine;
}

juce::String SlotLoader::getBlob (int slot) const
{
    if (! validSlot (slot))
        return {};
    const juce::ScopedLock sl (lock);
    return records[(size_t) slot].blob;
}
} // namespace ogi
