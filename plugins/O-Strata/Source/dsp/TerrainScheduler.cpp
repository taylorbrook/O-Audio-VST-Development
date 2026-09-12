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
#include "TerrainImage.h"
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

    JobStatus runJob() override
    {
        if (shouldExit() || cancel.load())
            return jobHasFinished;
        jassert (! scheduler.processor.isInsideProcessBlockOnThisThread());   // plan Decision 39

        auto built = scheduler.buildImage (osc, key, &cancel);
        if (built == nullptr || shouldExit() || cancel.load())
            return jobHasFinished;

        auto box = std::make_shared<std::unique_ptr<TerrainImage>> (std::move (built));
        std::weak_ptr<AliveToken> weak = scheduler.alive;
        TerrainScheduler* s = &scheduler;
        const int o = osc;
        juce::MessageManager::callAsync ([weak, s, o, box]
        {
            if (auto a = weak.lock())
                s->publishImage (o, std::move (*box));
        });
        scheduler.jobsCompleted.fetch_add (1);
        return jobHasFinished;
    }

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
        // Stage 3 D3 top-note key (plan Decision 13)
        pPos[osc]      = apvts.getRawParameterValue (pre + "Pos");
        pAspect[osc]   = apvts.getRawParameterValue (pre + "OrbAspect");
        pRot[osc]      = apvts.getRawParameterValue (pre + "OrbRot");
        pCX[osc]       = apvts.getRawParameterValue (pre + "OrbCX");
        pCY[osc]       = apvts.getRawParameterValue (pre + "OrbCY");
        pOrbMod[osc]   = apvts.getRawParameterValue (pre + "OrbMod");
        pCoarse[osc]   = apvts.getRawParameterValue (pre + "Coarse");
        pFine[osc]     = apvts.getRawParameterValue (pre + "Fine");
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

    refreshTopNote (osc, fs, orbit);
}

// ═══════════════════════════════════════════════════════════════════
// Stage 3 D3: "silent above <note>" (plan Decision 13; RESEARCH §2.4)
// ═══════════════════════════════════════════════════════════════════

void TerrainScheduler::refreshTopNote (int osc, double fs, OrbitKind orbit)
{
    // Only Bandlimited with a published set that stands for the current terrain (the
    // oscillator's own rule in updateBlockRate: key.terrain must match, else it plays
    // the analytic 1× path and nothing is muted).
    const int quality = static_cast<int> (pQuality[osc]->load());
    const int terrainIdx = juce::jlimit (0, 6, static_cast<int> (pTerrain[osc]->load()));
    const ChebyshevSet* set = nullptr;
    int generation = -1;
    if (quality == static_cast<int> (Quality::Bandlimited))
    {
        if (terrainIdx == static_cast<int> (TerrainKind::Imported))
        {
            if (const auto* img = processor.imagePtr[osc].load (std::memory_order_acquire))
            {
                set = &img->cheb;
                generation = processor.imageGeneration[osc].load (std::memory_order_acquire);
            }
        }
        else if (const auto* published = processor.chebPtr[osc].load (std::memory_order_acquire))
        {
            if (published->key.terrain == terrainIdx)
            {
                set = published;
                generation = processor.chebGeneration[osc].load (std::memory_order_acquire);
            }
        }
    }
    if (set == nullptr)
    {
        processor.chebTopNote[osc].store (-1, std::memory_order_relaxed);
        topNoteKeyValid[osc] = false;
        return;
    }

    auto q = [] (float v) { return static_cast<int> (std::lround (v * 1024.0f)); };
    TopNoteKey key;
    key.generation = generation;
    key.orbit = static_cast<int> (orbit);
    key.coarse = static_cast<int> (pCoarse[osc]->load());
    key.fine = q (pFine[osc]->load() / 100.0f);
    key.pos = q (pPos[osc]->load());
    key.aspect = q (pAspect[osc]->load());
    key.rot = q (pRot[osc]->load() / 360.0f);
    key.cx = q (pCX[osc]->load());
    key.cy = q (pCY[osc]->load());
    key.mod = q (pOrbMod[osc]->load());
    key.fs = static_cast<int> (std::lround (fs));

    const bool periodic = ++topNotePolls[osc] >= kTopNoteRefreshPolls;
    if (topNoteKeyValid[osc] && key == topNoteKey[osc] && ! periodic)
        return;
    topNotePolls[osc] = 0;
    topNoteKey[osc] = key;
    topNoteKeyValid[osc] = true;

    // Bisection over MIDI 0–127 for the highest sounding note: A(n) is non-increasing in
    // n (the taper only removes diagonals as f rises), so ≈ 7 probes (≈ 0.6 ms) settle it.
    const int K = orbitKNominal (orbit);
    auto sounds = [&] (int n) {
        return probeHarmonicAmplitude (osc, *set, orbit, K, fs, processor.tunedFrequency (n, osc)) >= kTopNoteTau;
    };
    int top;
    if (sounds (127))       top = 127;
    else if (! sounds (0))  top = -1;   // nothing sounds at any pitch — no note to name
    else
    {
        int lo = 0, hi = 127;           // invariant: lo sounds, hi is muted
        while (hi - lo > 1)
        {
            const int mid = (lo + hi) / 2;
            if (sounds (mid)) lo = mid; else hi = mid;
        }
        top = lo;
    }
    processor.chebTopNote[osc].store (top >= kTopNoteHide ? -1 : top, std::memory_order_relaxed);
}

double TerrainScheduler::probeHarmonicAmplitude (int osc, const ChebyshevSet& set, OrbitKind orbit, int K, double fs, double f) const
{
    // The voice's tapered copy (TerrainOscillator::buildChebWeights) at this pitch
    const double dc = chebDiagonalCutoff (fs, K, f);
    float c[kChebCoeffs];
    for (int n = 0; n <= kChebDegree; ++n)
    {
        const int row = chebRowStart (n);
        for (int m = 0; m + n <= kChebDegree; ++m)
            c[row + m] = set.c[static_cast<size_t> (row + m)] * chebTaperWeight (n + m, dc);
    }

    // The base orbit at the raw (unmodulated) orbit parameters, then the voice's affine
    // (aspect on y, rotation, size, centre, clamp) — no feedback, no saturation.
    const float m = juce::jlimit (0.0f, 1.0f, pOrbMod[osc]->load());
    const float aspect = juce::jlimit (0.1f, 1.0f, pAspect[osc]->load());
    const float rotRad = pRot[osc]->load() * 0.017453292519943295f;
    const float r = 0.05f + 0.95f * juce::jlimit (0.0f, 1.0f, pPos[osc]->load());
    const float cx = juce::jlimit (-1.0f, 1.0f, pCX[osc]->load());
    const float cy = juce::jlimit (-1.0f, 1.0f, pCY[osc]->load());
    const float sinRot = std::sin (rotRad), cosRot = std::cos (rotRad);
    const float* lut = SuperellipseLUT::get();
    OrbitScratch scratch;
    scratch.invTanhK = orbit == OrbitKind::Squarcle ? 1.0f / std::tanh (0.3f + 6.0f * m) : 1.0f;
    scratch.normFactor = 1.0f / OrbitDetail::maxRadius (orbit, m, scratch, lut);

    constexpr int N = 256;
    float y[N];
    for (int i = 0; i < N; ++i)
    {
        const float theta = static_cast<float> (6.283185307179586 * double (i) / double (N));
        const OrbitPoint b = baseOrbit (orbit, theta, m, scratch, lut);
        const float bx = b.x, by = b.y * aspect;
        const float px = juce::jlimit (-1.0f, 1.0f, (bx * cosRot - by * sinRot) * r + cx);
        const float py = juce::jlimit (-1.0f, 1.0f, (bx * sinRot + by * cosRot) * r + cy);
        const float v = clenshaw2D (c, px, py);
        y[i] = std::isfinite (v) ? v : 0.0f;
    }

    // 256-point DFT, strongest harmonic k >= 1: A = 2 |X_k| / N (one orbit cycle = one period)
    double best = 0.0;
    for (int k = 1; k <= N / 2; ++k)
    {
        double re = 0.0, im = 0.0;
        const double w = 6.283185307179586 * double (k) / double (N);
        for (int i = 0; i < N; ++i)
        {
            re += y[i] * std::cos (w * i);
            im -= y[i] * std::sin (w * i);
        }
        const double a = 2.0 * std::sqrt (re * re + im * im) / double (N);
        if (a > best) best = a;
    }
    return best;
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
// Image side (ARCH Core 8; plan Decisions 32, 35–38)
// ═══════════════════════════════════════════════════════════════════

bool TerrainScheduler::buildImageKey (int osc, ImageKey& out)
{
    // Hashed whenever an import exists (not only while the terrain is Imported), so
    // the image is ready when the user switches — plan Task 11's recorded decision.
    const int revision = processor.importRevision[osc].load (std::memory_order_acquire);
    if (revision <= 0)
        return false;
    out.revision = revision;
    out.blurQ = static_cast<int> (std::lround (juce::jlimit (0.0f, 1.0f, pBlur[osc]->load()) * 1024.0f));
    out.edge = juce::jlimit (0, 1, static_cast<int> (pEdge[osc]->load()));
    return true;
}

std::unique_ptr<TerrainImage> TerrainScheduler::buildImage (int osc, const ImageKey& key, const std::atomic<bool>* cancel)
{
    const auto slot = processor.getImportSlotCopy (osc);
    if (slot.decoded == nullptr || slot.revision != key.revision)
        return nullptr;   // superseded by a newer import
    return TerrainImage::build (slot.decoded, static_cast<float> (key.blurQ) / 1024.0f, static_cast<EdgeMode> (key.edge),
                                key.revision, slot.name, slot.sha256, cancel);
}

void TerrainScheduler::submitImage (int osc, const ImageKey& key)
{
    auto job = std::make_unique<ImageJob> (*this, osc, key);
    image[osc].inFlight = job.get();
    pool.addJob (job.get(), false);
    jobs.push_back (std::move (job));

    int live = 0;
    for (const auto& j : jobs)
        if (auto* ij = dynamic_cast<ImageJob*> (j.get()))
            if (ij->osc == osc && ! ij->cancel.load()) ++live;
    if (live > jobsInFlightMax[osc].load()) jobsInFlightMax[osc].store (live);
}

void TerrainScheduler::importInline (int osc, const ImageKey& key)
{
    jassert (! processor.isInsideProcessBlockOnThisThread());
    publishImage (osc, buildImage (osc, key, nullptr));
}

void TerrainScheduler::publishImage (int osc, std::unique_ptr<TerrainImage> img)
{
    JUCE_ASSERT_MESSAGE_THREAD
    if (img == nullptr) return;
    auto& s = image[osc];
    const ImageKey key { img->cheb.key.imageRevision, static_cast<int> (std::lround (img->blur * 1024.0f)), static_cast<int> (img->edge) };
    if (! s.seenValid || key != s.lastSeen || (s.publishedValid && key == s.lastPublished))
    {
        resultsDropped.fetch_add (1);
        return;
    }
    s.lastPublished = key; s.publishedValid = true;
    processor.imageFit[osc].store (img->fit, std::memory_order_relaxed);
    const TerrainImage* old = processor.imagePtr[osc].exchange (img.release(), std::memory_order_acq_rel);
    processor.retire (std::unique_ptr<Retirable> (const_cast<TerrainImage*> (old)));
    processor.imageGeneration[osc].fetch_add (1, std::memory_order_release);
    // Bandlimited + Imported: the image's embedded set IS the oscillator's set — mirror the readouts
    if (static_cast<int> (pQuality[osc]->load()) == static_cast<int> (Quality::Bandlimited)
        && juce::jlimit (0, 6, static_cast<int> (pTerrain[osc]->load())) == static_cast<int> (TerrainKind::Imported))
    {
        processor.chebFit[osc].store (processor.imageFit[osc].load (std::memory_order_relaxed), std::memory_order_relaxed);
        processor.chebGeneration[osc].fetch_add (1, std::memory_order_release);
    }
    processor.publishCount.fetch_add (1, std::memory_order_release);
    ++publishedThisPoll;
}
