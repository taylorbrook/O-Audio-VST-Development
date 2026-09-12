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

    TerrainScheduler.h
    O-Strata - Microtonal Wave-Terrain Synthesizer
    Ouaricon Audio

    The off-audio-thread side of Bandlimited mode and of the PNG path
    (ARCHITECTURE Core 7 + the Core 8 job side; plan Decisions 31, 32, 34, 39 —
    ARCH's `ChebyshevScheduler` name is a recorded rename: one class schedules
    both kinds of job).

    Every 50 ms (message-thread juce::Timer) it hashes, per oscillator, a ChebKey
    (terrain, Terrain Freq, Mod X, Mod Y — ModWheel / Aftertouch folded in through
    its own ModulationMatrix, values quantised to 1/1024, F clamped to [0.25, 2])
    while that oscillator is Bandlimited with an analytic terrain, and an ImageKey
    (import revision, Blur, Edge) whenever an import exists. A changed key is
    submitted to a 2-thread juce::ThreadPool at most once per debounce period
    (50 ms cheb / 150 ms image; intermediate keys are superseded; a job still in
    flight for an older key is signalled to exit). A finished job posts its result
    with MessageManager::callAsync guarded by a weak AliveToken, and publish()
    (message thread) swaps the processor's atomic pointer, retires the old object
    through the type-erased reaper, and bumps the readout atomics. Results whose
    key is no longer the last one seen are dropped.

    runOnceSynchronously() is the harness's deterministic path: one poll, inline
    projection / import on the calling (message) thread, direct publish, no
    debounce, no pool. Production never calls it.

    shutdown() (stop timer, remove all jobs, reset the token) is the FIRST call in
    the processor destructor; the scheduler is the processor's LAST data member
    so it is destroyed first.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "ChebyshevSet.h"
#include "ModulationMatrix.h"
#include "Orbits.h"
#include <atomic>
#include <memory>
#include <vector>

class OStrataAudioProcessor;
struct TerrainImage;
struct DecodedImage;

class TerrainScheduler : private juce::Timer
{
public:
    explicit TerrainScheduler (OStrataAudioProcessor& p);
    ~TerrainScheduler() override;

    static constexpr int kPollMs = 50;
    static constexpr int kChebDebounceMs = 50;
    static constexpr int kImageDebounceMs = 150;
    static constexpr float kBandlimitedMaxF = 2.0f;   // CONTEXT D4: one set per oscillator, no F-lattice (v1.1)

    /** Message thread. Stops the timer, removes every job (interrupting, 2 s), resets the token. */
    void shutdown();

    /** HARNESS ONLY (message thread): one poll, inline jobs, direct publish; no
        debounce, no pool. Returns the number of objects published. */
    int runOnceSynchronously();

    /** HARNESS ONLY: publish a hand-built set directly (H1 "Chebyshev identity" row);
        no key check, the set's key becomes the last published one. */
    void publishForHarness (int osc, std::unique_ptr<ChebyshevSet> set);

    // ─── Counters (harness reads; data only) ───
    std::atomic<int> jobsInFlightMax[2] { 0, 0 };   // max simultaneous non-cancelled jobs per oscillator
    std::atomic<int> jobsCancelled { 0 };            // in-flight jobs signalled to exit by a newer key
    std::atomic<int> jobsCompleted { 0 };
    std::atomic<int> keysSuperseded { 0 };           // keys observed but replaced before they could be submitted
    std::atomic<int> resultsDropped { 0 };           // results whose key was stale at publish
    int getLiveJobCount() const { return static_cast<int> (jobs.size()); }

private:
    struct AliveToken {};
    struct ChebJob;
    struct ImageJob;
    friend struct ChebJob;
    friend struct ImageJob;

    struct ChebSlot
    {
        ChebKey lastSeen, lastPublished, pending;
        bool seenValid = false, publishedValid = false, pendingValid = false;
        double lastSubmitMs = -1.0e9;
        ChebJob* inFlight = nullptr;
    };

    struct ImageKey
    {
        int revision = 0, blurQ = 0, edge = 0;
        bool operator== (const ImageKey& o) const noexcept { return revision == o.revision && blurQ == o.blurQ && edge == o.edge; }
        bool operator!= (const ImageKey& o) const noexcept { return ! (*this == o); }
    };
    struct ImageSlot
    {
        ImageKey lastSeen, lastPublished, pending;
        bool seenValid = false, publishedValid = false, pendingValid = false;
        double lastSubmitMs = -1.0e9;
        ImageJob* inFlight = nullptr;
    };

    void timerCallback() override;
    void poll (bool synchronous);

    // Cheb side
    bool buildChebKey (int osc, ChebKey& out);          // false when this oscillator hashes nothing
    void projectInline (int osc, const ChebKey& key);   // synchronous path
    void submitCheb (int osc, const ChebKey& key);
    void publish (int osc, std::unique_ptr<ChebyshevSet> set);

    // Image side (Phase 2.5)
    bool buildImageKey (int osc, ImageKey& out);
    void importInline (int osc, const ImageKey& key);
    void submitImage (int osc, const ImageKey& key);
    void publishImage (int osc, std::unique_ptr<TerrainImage> image);
    std::unique_ptr<TerrainImage> buildImage (int osc, const ImageKey& key, const std::atomic<bool>* cancel);

    void refreshReadouts (int osc);

    // ─── Stage 3 D3 top-note probe (plan Decision 13; RESEARCH §2.4) ───
    struct TopNoteKey
    {
        int generation = -1, orbit = -1, coarse = 0, fine = 0;
        int pos = 0, aspect = 0, rot = 0, cx = 0, cy = 0, mod = 0;   // raw values quantised to 1/1024 (rot: turns)
        int fs = 0;
        bool operator== (const TopNoteKey& o) const noexcept
        {
            return generation == o.generation && orbit == o.orbit && coarse == o.coarse && fine == o.fine
                && pos == o.pos && aspect == o.aspect && rot == o.rot && cx == o.cx && cy == o.cy && mod == o.mod && fs == o.fs;
        }
        bool operator!= (const TopNoteKey& o) const noexcept { return ! (*this == o); }
    };
    static constexpr int kTopNoteRefreshPolls = 10;        // 500 ms — covers tuning-table changes
    static constexpr double kTopNoteTau = 2.0e-3;           // ≈ −54 dBFS strongest harmonic (17 / 17 muted rows)
    static constexpr int kTopNoteHide = 108;                // C8: top >= this → −1 (nothing to show)
    void refreshTopNote (int osc, double fs, OrbitKind orbit);
    /** Strongest-harmonic amplitude A(n) of the tapered `set` traced on the base orbit at f (Hz). */
    double probeHarmonicAmplitude (int osc, const ChebyshevSet& set, OrbitKind orbit, int K, double fs, double f) const;
    TopNoteKey topNoteKey[2];
    bool topNoteKeyValid[2] = { false, false };
    int topNotePolls[2] = { 0, 0 };

    void pruneFinishedJobs();
    static double nowMs();
    int publishedThisPoll = 0;

    OStrataAudioProcessor& processor;
    juce::ThreadPool pool;
    ModulationMatrix foldMatrix;
    ChebSlot cheb[2];
    ImageSlot image[2];
    std::vector<std::unique_ptr<juce::ThreadPoolJob>> jobs;   // owned here; pruned once the pool has finished them
    std::shared_ptr<AliveToken> alive;

    // Cached raw parameter pointers (message-thread reads)
    std::atomic<float>* pQuality[2] = {};
    std::atomic<float>* pTerrain[2] = {};
    std::atomic<float>* pTerFreq[2] = {};
    std::atomic<float>* pTerModX[2] = {};
    std::atomic<float>* pTerModY[2] = {};
    std::atomic<float>* pOrbit[2] = {};
    std::atomic<float>* pFeedback[2] = {};
    std::atomic<float>* pBlur[2] = {};
    std::atomic<float>* pEdge[2] = {};
    // D3 top-note key inputs (Stage 3)
    std::atomic<float>* pPos[2] = {};
    std::atomic<float>* pAspect[2] = {};
    std::atomic<float>* pRot[2] = {};
    std::atomic<float>* pCX[2] = {};
    std::atomic<float>* pCY[2] = {};
    std::atomic<float>* pOrbMod[2] = {};
    std::atomic<float>* pCoarse[2] = {};
    std::atomic<float>* pFine[2] = {};

    JUCE_DECLARE_NON_COPYABLE (TerrainScheduler)
};
