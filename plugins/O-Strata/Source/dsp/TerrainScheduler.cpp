/*
   This file is part of O-Strata, an Ouaricon Audio plugin.
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

    TerrainScheduler.cpp
    O-Strata - Microtonal Wave-Terrain Synthesizer
    Ouaricon Audio

    Message thread + the pool. Nothing here runs on the audio thread (plan
    Decision 39: every job asserts it is not inside processBlock on its thread).

  ==============================================================================
*/

#include "TerrainScheduler.h"
#include "ChebyshevProjector.h"
#include "Orbits.h"
#include "../PluginProcessor.h"
#include <chrono>
#include <thread>

// ═══════════════════════════════════════════════════════════════════
// Jobs
// ═══════════════════════════════════════════════════════════════════

struct TerrainScheduler::ChebJob final : juce::ThreadPoolJob
{
    ChebJob (TerrainScheduler& s, int o, const ChebKey& k)
        : juce::ThreadPoolJob ("O-Strata cheb"), scheduler (s), osc (o), key (k) {}

    JobStatus runJob() override
    {
        if (shouldExit() || cancel.load())
            return jobHasFinished;
        jassert (! scheduler.processor.isInsideProcessBlockOnThisThread());   // plan Decision 39

        auto set = std::make_unique<ChebyshevSet>();
        set->key = key;
        ChebyshevProjector::projectAnalytic (static_cast<TerrainKind> (key.terrain), key.F, key.modX, key.modY, *set, &cancel);
        if (shouldExit() || cancel.load())
            return jobHasFinished;

        // callAsync needs a copyable callable: box the unique_ptr in a shared_ptr.
        // If the message is never delivered (shutdown), the box frees the set.
        auto box = std::make_shared<std::unique_ptr<ChebyshevSet>> (std::move (set));
        std::weak_ptr<AliveToken> weak = scheduler.alive;
        TerrainScheduler* s = &scheduler;
        const int o = osc;
        juce::MessageManager::callAsync ([weak, s, o, box]
        {
            if (auto a = weak.lock())
                s->publish (o, std::move (*box));
        });
        scheduler.jobsCompleted.fetch_add (1);
        return jobHasFinished;
    }

    TerrainScheduler& scheduler;
    int osc;
    ChebKey key;
    std::atomic<bool> cancel { false };
};

struct TerrainScheduler::ImageJob final : juce::ThreadPoolJob
{
    ImageJob (TerrainScheduler& s, int o, const ImageKey& k)
        : juce::ThreadPoolJob ("O-Strata image"), scheduler (s), osc (o), key (k) {}

    JobStatus runJob() override;

    TerrainScheduler& scheduler;
    int osc;
    ImageKey key;
    std::atomic<bool> cancel { false };
};

// ═══════════════════════════════════════════════════════════════════
// Construction / shutdown
// ═══════════════════════════════════════════════════════════════════

TerrainScheduler::TerrainScheduler (OStrataAudioProcessor& p)
    : processor (p),
      pool (juce::ThreadPoolOptions().withThreadName ("O-Strata DSP").withNumberOfThreads (2)),
      alive (std::make_shared<AliveToken>())
{
    auto& apvts = processor.getAPVTS();
    foldMatrix.setAPVTS (&apvts);
    for (int osc = 0; osc < 2; ++osc)
    {
        const juce::String pre = osc == 0 ? "oscA" : "oscB";
        pQuality[osc]  = apvts.getRawParameterValue (pre + "Quality");
        pTerrain[osc]  = apvts.getRawParameterValue (pre + "Terrain");
        pTerFreq[osc]  = apvts.getRawParameterValue (pre + "TerFreq");
        pTerModX[osc]  = apvts.getRawParameterValue (pre + "TerModX");
        pTerModY[osc]  = apvts.getRawParameterValue (pre + "TerModY");
        pOrbit[osc]    = apvts.getRawParameterValue (pre + "Orbit");
        pFeedback[osc] = apvts.getRawParameterValue (pre + "OrbFeedback");
        pBlur[osc]     = apvts.getRawParameterValue (pre + "TerBlur");
        pEdge[osc]     = apvts.getRawParameterValue (pre + "TerEdge");
    }
    startTimer (kPollMs);
}

TerrainScheduler::~TerrainScheduler()
{
    shutdown();
}

void TerrainScheduler::shutdown()
{
    stopTimer();
    pool.removeAllJobs (true, 2000);
    jobs.clear();
    for (auto& s : cheb)  s.inFlight = nullptr;
    for (auto& s : image) s.inFlight = nullptr;
    alive.reset();
}

double TerrainScheduler::nowMs()
{
    return std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now().time_since_epoch()).count();
}

void TerrainScheduler::pruneFinishedJobs()
{
    for (size_t i = 0; i < jobs.size();)
    {
        if (pool.contains (jobs[i].get()))
            ++i;
        else
        {
            for (auto& s : cheb)  if (s.inFlight == jobs[i].get())  s.inFlight = nullptr;
            for (auto& s : image) if (s.inFlight == jobs[i].get()) s.inFlight = nullptr;
            jobs.erase (jobs.begin() + static_cast<long> (i));
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
// Polling
// ═══════════════════════════════════════════════════════════════════

void TerrainScheduler::timerCallback()
{
    poll (false);
}

int TerrainScheduler::runOnceSynchronously()
{
    JUCE_ASSERT_MESSAGE_THREAD
    publishedThisPoll = 0;
    poll (true);
    return publishedThisPoll;
}

bool TerrainScheduler::buildChebKey (int osc, ChebKey& out)
{
    // Hashed only while this oscillator is Bandlimited AND its terrain is analytic
    // (0–5). `Imported` uses the image's embedded set (plan Decision 35).
    const int quality = static_cast<int> (pQuality[osc]->load());
    const int terrainIdx = juce::jlimit (0, 6, static_cast<int> (pTerrain[osc]->load()));
    if (quality != static_cast<int> (Quality::Bandlimited) || terrainIdx >= kNumAnalyticTerrains)
        return false;

    // Fold the two global sources through the scheduler's own matrix (plan Decision 31)
    const int base = static_cast<int> (osc == 0 ? ModDest::OscATerFreq : ModDest::OscBTerFreq);
    const float offF = foldMatrix.getModOffset (static_cast<ModDest> (base));
    const float offX = foldMatrix.getModOffset (static_cast<ModDest> (base + 1));
    const float offY = foldMatrix.getModOffset (static_cast<ModDest> (base + 2));

    auto q = [] (float v) { return static_cast<float> (std::round (v * 1024.0f) / 1024.0f); };   // 1/1024 quantisation
    const float F = juce::jlimit (0.25f, kBandlimitedMaxF, std::exp2 (std::log2 (pTerFreq[osc]->load()) + 2.0f * offF));
    out.terrain = terrainIdx;
    out.F = q (F);
    out.modX = q (juce::jlimit (0.0f, 1.0f, pTerModX[osc]->load() + offX));
    out.modY = q (juce::jlimit (0.0f, 1.0f, pTerModY[osc]->load() + offY));
    out.imageRevision = 0;
    return true;
}

void TerrainScheduler::refreshReadouts (int osc)
{
    double fs = processor.getSampleRate();
    if (fs <= 0.0) fs = 48000.0;
    const auto orbit = static_cast<OrbitKind> (juce::jlimit (0, kNumOrbitKinds - 1, static_cast<int> (pOrbit[osc]->load())));
    const int K = orbitKNominal (orbit);
    const int dmax = chebDMax (chebDiagonalCutoff (fs, K, 261.6256));
    processor.chebPartialsAtC4[osc].store (juce::jmin (kChebDegree, dmax) * K, std::memory_order_relaxed);

    const bool fbRouted = foldMatrix.isDestinationRouted (osc == 0 ? ModDest::OscAOrbFeedback : ModDest::OscBOrbFeedback);
    const bool approximate = orbitK (orbit) == 0 || pFeedback[osc]->load() > 0.0f || fbRouted;
    processor.chebApproximate[osc].store (approximate, std::memory_order_relaxed);
}

void TerrainScheduler::poll (bool synchronous)
{
    JUCE_ASSERT_MESSAGE_THREAD
    pruneFinishedJobs();

    foldMatrix.updateFromAPVTS();
    foldMatrix.setSourceValue (ModSource::ModWheel, processor.getModWheelValue());
    foldMatrix.setSourceValue (ModSource::Aftertouch, processor.getAftertouchValue());
    foldMatrix.evaluate();

    const double now = nowMs();

    for (int osc = 0; osc < 2; ++osc)
    {
        refreshReadouts (osc);

        // ── Chebyshev sets ──
        ChebKey key;
        auto& s = cheb[osc];
        if (buildChebKey (osc, key))
        {
            s.lastSeen = key; s.seenValid = true;
            const bool wanted = ! (s.publishedValid && key == s.lastPublished);
            if (wanted)
            {
                if (synchronous)
                {
                    projectInline (osc, key);
                }
                else
                {
                    if (s.inFlight != nullptr && s.inFlight->key != key)
                    {
                        s.inFlight->cancel.store (true);
                        s.inFlight->signalJobShouldExit();
                        s.inFlight = nullptr;
                        jobsCancelled.fetch_add (1);
                    }
                    if (s.inFlight == nullptr)
                    {
                        if (now - s.lastSubmitMs >= kChebDebounceMs)
                        {
                            if (s.pendingValid && s.pending != key) keysSuperseded.fetch_add (1);
                            s.pendingValid = false;
                            submitCheb (osc, key);
                            s.lastSubmitMs = now;
                        }
                        else
                        {
                            if (s.pendingValid && s.pending != key) keysSuperseded.fetch_add (1);
                            s.pending = key; s.pendingValid = true;
                        }
                    }
                }
            }
        }
        else
        {
            s.seenValid = false;
        }

        // ── Images (Phase 2.5) ──
        ImageKey ik;
        auto& is = image[osc];
        if (buildImageKey (osc, ik))
        {
            is.lastSeen = ik; is.seenValid = true;
            const bool wanted = ! (is.publishedValid && ik == is.lastPublished);
            if (wanted)
            {
                if (synchronous)
                {
                    importInline (osc, ik);
                }
                else
                {
                    if (is.inFlight != nullptr && is.inFlight->key != ik)
                    {
                        is.inFlight->cancel.store (true);
                        is.inFlight->signalJobShouldExit();
                        is.inFlight = nullptr;
                        jobsCancelled.fetch_add (1);
                    }
                    if (is.inFlight == nullptr)
                    {
                        if (now - is.lastSubmitMs >= kImageDebounceMs)
                        {
                            if (is.pendingValid && is.pending != ik) keysSuperseded.fetch_add (1);
                            is.pendingValid = false;
                            submitImage (osc, ik);
                            is.lastSubmitMs = now;
                        }
                        else
                        {
                            if (is.pendingValid && is.pending != ik) keysSuperseded.fetch_add (1);
                            is.pending = ik; is.pendingValid = true;
                        }
                    }
                }
            }
        }
        else
        {
            is.seenValid = false;
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
// Chebyshev side
// ═══════════════════════════════════════════════════════════════════

void TerrainScheduler::submitCheb (int osc, const ChebKey& key)
{
    auto job = std::make_unique<ChebJob> (*this, osc, key);
    cheb[osc].inFlight = job.get();
    pool.addJob (job.get(), false);   // owned by `jobs`, never by the pool
    jobs.push_back (std::move (job));

    int live = 0;
    for (const auto& j : jobs)
        if (auto* cj = dynamic_cast<ChebJob*> (j.get()))
            if (cj->osc == osc && ! cj->cancel.load()) ++live;
    if (live > jobsInFlightMax[osc].load()) jobsInFlightMax[osc].store (live);
}

void TerrainScheduler::projectInline (int osc, const ChebKey& key)
{
    jassert (! processor.isInsideProcessBlockOnThisThread());
    auto set = std::make_unique<ChebyshevSet>();
    set->key = key;
    ChebyshevProjector::projectAnalytic (static_cast<TerrainKind> (key.terrain), key.F, key.modX, key.modY, *set);
    publish (osc, std::move (set));
}

void TerrainScheduler::publish (int osc, std::unique_ptr<ChebyshevSet> set)
{
    JUCE_ASSERT_MESSAGE_THREAD
    if (set == nullptr) return;
    auto& s = cheb[osc];
    // Superseded (a newer key was seen since) or already published (the sync path
    // beat the async result for the same key): drop.
    if (! s.seenValid || set->key != s.lastSeen || (s.publishedValid && set->key == s.lastPublished))
    {
        resultsDropped.fetch_add (1);
        return;
    }
    s.lastPublished = set->key; s.publishedValid = true;
    processor.chebFit[osc].store (set->fit, std::memory_order_relaxed);
    const ChebyshevSet* old = processor.chebPtr[osc].exchange (set.release(), std::memory_order_acq_rel);
    processor.retire (std::unique_ptr<Retirable> (const_cast<ChebyshevSet*> (old)));
    processor.chebGeneration[osc].fetch_add (1, std::memory_order_release);
    processor.publishCount.fetch_add (1, std::memory_order_release);
    ++publishedThisPoll;
}

void TerrainScheduler::publishForHarness (int osc, std::unique_ptr<ChebyshevSet> set)
{
    JUCE_ASSERT_MESSAGE_THREAD
    if (set == nullptr) return;
    auto& s = cheb[osc];
    s.lastSeen = s.lastPublished = set->key; s.seenValid = s.publishedValid = true;
    processor.chebFit[osc].store (set->fit, std::memory_order_relaxed);
    const ChebyshevSet* old = processor.chebPtr[osc].exchange (set.release(), std::memory_order_acq_rel);
    processor.retire (std::unique_ptr<Retirable> (const_cast<ChebyshevSet*> (old)));
    processor.chebGeneration[osc].fetch_add (1, std::memory_order_release);
    processor.publishCount.fetch_add (1, std::memory_order_release);
}

// ═══════════════════════════════════════════════════════════════════
// Image side — Phase 2.5 (Task 11) fills these in
// ═══════════════════════════════════════════════════════════════════

bool TerrainScheduler::buildImageKey (int, ImageKey&) { return false; }
void TerrainScheduler::importInline (int, const ImageKey&) {}
void TerrainScheduler::submitImage (int, const ImageKey&) {}
juce::ThreadPoolJob::JobStatus TerrainScheduler::ImageJob::runJob() { return jobHasFinished; }
