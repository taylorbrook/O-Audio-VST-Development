/*
   This file is part of O-Granulator, an Ouaricon Audio plugin.
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
#include "Engine.h"

#include <algorithm>

namespace ogran
{
//==============================================================================
// Message thread
//==============================================================================
void Engine::prepare (double sampleRate, int maxBlockSize)
{
    juce::ignoreUnused (maxBlockSize);   // nothing in process() scales with the host block

    fs          = sampleRate > 0.0 ? sampleRate : 48000.0;
    rampCells   = rampCellsFor (fs);
    mixerRampCells = rampCellsFor (fs, kMixerRampMs);
    ringSamples = kRingSamples (fs);

    // D2: full 41 s at every rate. prepare() re-assigns the same size on a repeat
    // call (no reallocation) and clears 27.6 MB @ 48 k — message thread only.
    ring.prepare (ringSamples);
    pool.prepare (fs, kPoolFadeMs);
    spawner.prepare (fs);
    for (auto& s : scheds)
        s.prepare();   // epochs zeroed here and nowhere else (R4)

    for (auto& ch : fbDelay)
        std::fill (std::begin (ch), std::end (ch), 0.0f);
    for (auto& sb : bus)
        for (auto& ch : sb)
            std::fill (std::begin (ch), std::end (ch), 0.0f);

    absCount = 0;
    gp.latch (ptrs);
    spanSamples = static_cast<double> (gp.bufferLengthS) * fs;
    copyCustomTable (true);
    for (auto& a : activeSets)
        a.invalidate();
    updateScaleState();
    snapRampsToParams();
    activeSeed = pendingSeed;

   #if OGRAN_TEST_HOOKS
    diag.reset();
    armGuards();
   #endif

    prepared = true;
}

void Engine::snapRampsToParams() noexcept
{
    inGainRamp.reset (dbToGain (gp.inputGainDb));
    outGainRamp.reset (dbToGain (gp.outputGainDb));
    mixRamp.reset (gp.mix);

    const auto dw = mixGains (mixRamp.current());
    dryGain.reset (dw.a);
    wetGain.reset (dw.b);

    float gOut[kMaxStreams] {}, aud[kMaxStreams] {};
    computeStreamTargets (gOut, aud);
    for (int s = 0; s < kMaxStreams; ++s)
    {
        const auto& st = gp.s[s];
        levelRamp[s].reset (levelDbToGain (st.levelDb));
        panRamp[s].reset (st.pan);
        gOutRamp[s].reset (gOut[s]);
        audRamp[s].reset (aud[s]);

        const auto pg = panGains (panRamp[s].current());
        const float g = gOutRamp[s].current() * levelRamp[s].current() * audRamp[s].current();
        streamGainL[s].reset (g * pg.a);
        streamGainR[s].reset (g * pg.b);
    }
}

//==============================================================================
// 2.2 grid helpers
//==============================================================================
const PitchCollections::Table& Engine::tableForChoice (int choice) const noexcept
{
    const int id = scaleChoiceToId (choice);
    if (id == PitchCollections::Custom)
        return customCopy;
    return builtIns[static_cast<size_t> (std::clamp (id, 0, PitchCollections::kCount - 1))];
}

/** P11: copy the processor's custom table when its version moved. On the audio
    thread a NON-BLOCKING try-lock (a miss retries at the next grid); prepare()
    (message thread) may block. A copy taken just after a newer write is merely
    re-copied at the next grid (the version it is labelled with is older). */
void Engine::copyCustomTable (bool blocking) noexcept
{
    if (scaleSrc.lock == nullptr || scaleSrc.customTable == nullptr || scaleSrc.customVersion == nullptr)
        return;
    const std::uint32_t v = scaleSrc.customVersion->load (std::memory_order_acquire);
    if (blocking)
    {
        const juce::SpinLock::ScopedLockType sl (*scaleSrc.lock);
        customCopy = *scaleSrc.customTable;
        customCopyVersion = v;
        return;
    }
    if (v == customCopyVersion)
        return;
    const juce::SpinLock::ScopedTryLockType tl (*scaleSrc.lock);
    if (tl.isLocked())
    {
        customCopy = *scaleSrc.customTable;   // trivially copyable, ≈ 264 B
        customCopyVersion = v;
       #if OGRAN_TEST_HOOKS
        ++diag.customCopies;
       #endif
    }
   #if OGRAN_TEST_HOOKS
    else
    {
        ++diag.customTryFails;
    }
   #endif
}

/** Per enabled stream with scale ≠ Off: rebuild the active set if its key moved
    (T6), then the pick window for the current base pitch. Off / disabled →
    PitchSel {} (the 2.1 continuous path). */
void Engine::updateScaleState() noexcept
{
    for (int s = 0; s < kMaxStreams; ++s)
    {
        const auto& st = gp.s[s];
        if (! st.enable || st.scale <= 0)
        {
            pitchSel[s] = {};
            continue;
        }
        const std::uint64_t mask = scaleSrc.masks != nullptr
                                       ? scaleSrc.masks[scaleMaskIndex (s, st.scale)].load (std::memory_order_relaxed)
                                       : ~std::uint64_t (0);
        const float spanCents = st.pitchRand * 100.0f;
        if (activeSets[s].rebuildIfNeeded (st.scale, mask, spanCents, customCopyVersion, tableForChoice (st.scale)))
        {
           #if OGRAN_TEST_HOOKS
            ++diag.activeSetRebuilds;
           #endif
        }
        auto& ps = pitchSel[s];
        ps.set = &activeSets[s];
        activeSets[s].window (static_cast<double> (st.pitch), static_cast<double> (spanCents), ps.lo, ps.hi);
    }
}

/** Quotas (T5) → per stream R5 coherence + g_out (T4), and the mute / solo
    audibility mask. Latched parameters only (never pool occupancy). */
void Engine::computeStreamTargets (float (&gOut)[kMaxStreams], float (&audible)[kMaxStreams]) noexcept
{
    double demand[kMaxStreams] {};
    bool anySolo = false;
    for (int s = 0; s < kMaxStreams; ++s)
    {
        const auto& st = gp.s[s];
        if (st.enable)
            demand[s] = streamDemand (static_cast<double> (st.density), static_cast<double> (st.sizeMs), fs);
        anySolo = anySolo || (st.enable && st.solo);
    }
    quotas = waterFill (demand, gp.grainLimit);

    for (int s = 0; s < kMaxStreams; ++s)
    {
        const auto& st = gp.s[s];
        const bool scaleOn = pitchSel[s].set != nullptr;
        coherence[s] = Coherence::compute (static_cast<double> (st.reverseProb), static_cast<double> (st.spray),
                                           spanSamples, fs, static_cast<double> (st.pitch),
                                           static_cast<double> (st.pitchRand), scaleOn,
                                           scaleOn ? pitchSel[s].hi - pitchSel[s].lo : 0,
                                           scaleOn && pitchSel[s].set->hasUnison());

        // A disabled stream has demand 0 in the water-fill, but spawns nothing;
        // its ringing grains keep the 2.1 form (own N against the whole cap).
        WindowSel win;
        win.step = WindowMorphBank::stepFor (st.window);
        win.tilt = ouaricon::grain::WindowLut::makeTilt (0.5f + 0.45f * st.windowSkew);
        const double n = st.enable ? demand[s]
                                   : streamDemand (static_cast<double> (st.density), static_cast<double> (st.sizeMs), fs);
        const double q = st.enable ? quotas.q[s] : static_cast<double> (gp.grainLimit);
        gOut[s] = outputGainFromPower (streamPower (n, q, win, coherence[s], static_cast<double> (st.spread), bank));

        audible[s] = (! st.mute && (! anySolo || st.solo)) ? 1.0f : 0.0f;
    }
}

//==============================================================================
// Audio thread
//==============================================================================
void Engine::process (const float* const* in, int numIn, float* const* out, int numOut,
                      int numSamples, std::uint64_t seed) noexcept
{
    if (! prepared || numSamples <= 0 || numIn < 1 || numOut < 1 || in == nullptr || out == nullptr)
        return;

    pendingSeed = seed;

    // Engine input is stereo: a mono bus feeds L = R.
    const float* const inL  = in[0];
    const float* const inR  = numIn > 1 ? in[1] : in[0];
    float* const       outL = out[0];
    float* const       outR = numOut > 1 ? out[1] : nullptr;

    int done = 0;
    while (done < numSamples)   // bounded: every iteration consumes >= 1 sample
    {
        const int cellPos = static_cast<int> (absCount % kCell);
        const int len     = std::min (numSamples - done, kCell - cellPos);

        // 1. Control tick on the absolute grid.
        if (cellPos == 0)
            gridTick();

        // 3. Write before read.
        CaptureWriter::write (ring, inL + done, inR + done, len, cellPos, inGainRamp, fbDelay);
        jassert (ring.getWriteCount() == absCount + len);

        // 4. Schedule.
        const int numOnsets = scheduleStreams (len);

        // 5. Render grain-major, split at each onset; spawn at the split.
        for (auto& sb : bus)
            for (auto& ch : sb)
                std::fill (ch, ch + len, 0.0f);

        int seg = 0;
        for (int k = 0; k < numOnsets; ++k)
        {
            const auto& o = onsets[k];
            if (o.offset > seg)
            {
                renderSegment (seg, o.offset);
                seg = o.offset;
            }
            spawn (o, absCount);
        }
        if (seg < len)
            renderSegment (seg, len);

        // 6 + 8. Stream mix, dry/wet, output gain.
        mixOut (inL + done, inR + done, outL + done, outR != nullptr ? outR + done : nullptr, numOut, len, cellPos);

        absCount += len;
        done     += len;
    }
}

void Engine::gridTick() noexcept
{
    activeSeed = pendingSeed;
    gp.latch (ptrs);
    spanSamples = static_cast<double> (gp.bufferLengthS) * fs;   // spawn-latched destination (no ramp)

    // All four streams (2.2 removes the 2.1 P5 mask). Each keeps its R4 epoch and
    // rising-edge reset.
    for (int s = 0; s < kMaxStreams; ++s)
        scheds[s].setEnabled (gp.s[s].enable);

    // P11 custom-table copy (version-gated, non-blocking), then active sets.
    copyCustomTable (false);
    updateScaleState();

    inGainRamp.setTarget (dbToGain (gp.inputGainDb), rampCells);
    inGainRamp.tick();
    outGainRamp.setTarget (dbToGain (gp.outputGainDb), rampCells);
    outGainRamp.tick();
    mixRamp.setTarget (gp.mix, rampCells);
    mixRamp.tick();

    const auto dw = mixGains (mixRamp.current());   // snapped ends: exactly (1, 0) at mix 0
    dryGain.push (dw.a);
    wetGain.push (dw.b);

    // Quotas → coherence → g_out (20 ms); audibility, level, pan (10 ms, P12).
    float gOut[kMaxStreams] {}, aud[kMaxStreams] {};
    computeStreamTargets (gOut, aud);
    for (int s = 0; s < kMaxStreams; ++s)
    {
        const auto& st = gp.s[s];
        levelRamp[s].setTarget (levelDbToGain (st.levelDb), mixerRampCells);
        levelRamp[s].tick();
        panRamp[s].setTarget (st.pan, mixerRampCells);
        panRamp[s].tick();
        audRamp[s].setTarget (aud[s], mixerRampCells);
        audRamp[s].tick();
        gOutRamp[s].setTarget (gOut[s], rampCells);
        gOutRamp[s].tick();

        // (g_out · level) · audible: audible == 1.0f leaves the 2.1 product bitwise.
        const auto pg = panGains (panRamp[s].current());
        const float g = gOutRamp[s].current() * levelRamp[s].current() * audRamp[s].current();
        streamGainL[s].push (g * pg.a);
        streamGainR[s].push (g * pg.b);
    }
}

int Engine::scheduleStreams (int len) noexcept
{
    int counts[kMaxStreams] {};
    for (int s = 0; s < kMaxStreams; ++s)
    {
        if (! scheds[s].isEnabled())
            continue;
        const auto& st = gp.s[s];
        const double interval = fs / static_cast<double> (st.density);   // Free timing (2.5: Tempo)
        counts[s] = scheds[s].process (len, s, activeSeed, interval, st.jitter, streamOnsets[s]);
       #if OGRAN_TEST_HOOKS
        ++diag.schedCalls[s];
       #endif
    }
    return mergeOnsets (streamOnsets, counts, onsets);
}

#if OGRAN_TEST_HOOKS
// The S0 reference acquire: the pool's own policy, written independently of
// Spawner (its own progress lambda; never touches Spawner::pendingKill).
Spawner::Acquired Engine::nativeAcquireForTest() noexcept
{
    const Grain* const base = pool.getGrains().data();
    Spawner::Acquired a;
    a.grain = pool.tryObtain();
    if (a.grain == nullptr)
    {
        a.grain  = &pool.obtain ([] (const Grain& g) noexcept { return static_cast<double> (g.age) * g.invLen; });
        a.stolen = true;
    }
    a.slot = static_cast<int> (a.grain - base);
    int alive = 0;
    for (int i = 0; i < kPoolLive; ++i)
        if (pool.isActive (i)) ++alive;
    a.aliveAfter = alive;
    return a;
}
#endif

void Engine::spawn (const StreamOnset& o, std::int64_t pieceStart) noexcept
{
    bool forceLevel0 = false;
   #if OGRAN_TEST_HOOKS
    forceLevel0 = hooks.forceLevel0;
   #endif

    // Draw before obtain.
    const GrainPlan gpl = Spawner::plan (gp.s[o.stream], spanSamples, fs, ringSamples,
                                         activeSeed, o.stream, o.globalIndex, forceLevel0, pitchSel[o.stream]);
   #if OGRAN_TEST_HOOKS
    ++diag.planCalls[o.stream];
    AcquirePolicy policy;
    policy.nativeProgress = hooks.nativeProgress;
    policy.noQuotaCeil    = hooks.noQuotaCeil;
    const auto acq = hooks.nativeAcquire ? nativeAcquireForTest()
                                         : spawner.acquire (pool, gp.grainLimit, quotas, o.stream, policy);
   #else
    const auto acq = spawner.acquire (pool, gp.grainLimit, quotas, o.stream);
   #endif

    const std::int64_t onsetSample = pieceStart + o.offset;
    Spawner::fill (*acq.grain, gpl, o.stream, o.frac, onsetSample + 1);

   #if OGRAN_TEST_HOOKS
    ++diag.spawns;
    if (acq.killed) ++diag.kills;
    if (acq.stolen) ++diag.steals;
    diag.maxAlive = std::max (diag.maxAlive, static_cast<std::int64_t> (acq.aliveAfter));

    auto& rec = diag.log[static_cast<std::size_t> (diag.logCount % static_cast<std::uint64_t> (kSpawnLogSize))];
    rec.onsetSample = onsetSample;
    rec.writeHead   = onsetSample + 1;
    rec.globalIndex = o.globalIndex;
    rec.stream      = o.stream;
    rec.slot        = acq.slot;
    rec.dir         = gpl.dir;
    rec.level       = gpl.level;
    rec.set         = gpl.set;
    rec.delay       = gpl.delay;
    rec.anchor      = static_cast<double> (onsetSample + 1) - gpl.delay;
    rec.frac        = o.frac;
    rec.rate        = gpl.rate;
    rec.len         = gpl.len;
    rec.spanSamples = static_cast<float> (spanSamples);
    rec.grainLimit  = gp.grainLimit;
    rec.killed      = acq.killed;
    rec.stolen      = acq.stolen;
    rec.gainL       = gpl.gainL;
    rec.gainR       = gpl.gainR;
    rec.windowStep  = gpl.windowStep;
    rec.tiltT       = gpl.tilt.t;
    rec.pitchCents  = gpl.pitchCents;
    rec.quota       = quotas.q[o.stream];
    rec.quotaCeil   = quotas.qCeil[o.stream];
    ++diag.logCount;
   #endif
}

void Engine::renderSegment (int from, int to) noexcept
{
    // The ring head at render time (= end of this piece after step 3). Absolute
    // read positions do not depend on it; R1 keeps the delay round-trip exact.
    const std::int64_t head       = ring.getWriteCount();
    const bool         applyKills = spawner.hasPendingKills();
    const Grain* const base       = pool.getGrains().data();

    pool.render ([this, from, to, head, applyKills, base] (Grain& g, Pool::Fader& fade) noexcept -> bool
    {
        if (applyKills)
            spawner.applyPendingKill (g, base);

        float* const bl = bus[g.stream][0];
        float* const br = bus[g.stream][1];

        // P14: step 0 / skew 0 keeps the closed-form Hann (bitwise 2.1, R12);
        // every other window reads the bank (table + lerp + tilt warp). Latched
        // per grain, so the branch is constant for the whole grain.
        if (g.windowStep == 0 && juce::exactlyEqual (g.tiltT, 0.5f))
        {
            for (int i = from; i < to; ++i)
            {
                float l = 0.0f, r = 0.0f;
                readGrain (g, head, l, r);

                // Window LAST (after the 2.3 per-grain FX). killGain and fade are exactly
                // 1.0f for a live, non-killing grain — bitwise no-ops.
                const double phase = (static_cast<double> (g.age) + g.frac) * g.invLen;
                const float  w     = hannWindow (phase) * g.killGain * fade.next();
                bl[i] += w * l * g.gainL;
                br[i] += w * r * g.gainR;

                g.readAbs += g.step;
                ++g.age;
                if (g.killStep > 0.0f)
                    g.killGain = std::max (0.0f, g.killGain - g.killStep);

                if (! ((static_cast<double> (g.age) + g.frac) < g.len && g.killGain > 0.0f))
                    return false;   // retire
            }
            return true;
        }

        const float* const tab = bank.table (g.windowStep);
        const WindowMorphBank::Tilt tilt { g.tiltT, g.tiltA, g.tiltB };
        for (int i = from; i < to; ++i)
        {
            float l = 0.0f, r = 0.0f;
            readGrain (g, head, l, r);

            const double phase = (static_cast<double> (g.age) + g.frac) * g.invLen;
            const float  w     = WindowMorphBank::read (tab, tilt, phase) * g.killGain * fade.next();
            bl[i] += w * l * g.gainL;
            br[i] += w * r * g.gainR;

            g.readAbs += g.step;
            ++g.age;
            if (g.killStep > 0.0f)
                g.killGain = std::max (0.0f, g.killGain - g.killStep);

            if (! ((static_cast<double> (g.age) + g.frac) < g.len && g.killGain > 0.0f))
                return false;   // retire
        }
        return true;
    });

    if (applyKills)
        spawner.pendingKillsApplied();
}

void Engine::mixOut (const float* dryL, const float* dryR, float* outL, float* outR,
                     int numOut, int len, int cellPos) noexcept
{
    // P4 / R2: with mix settled at exactly 0 and output gain settled at exactly
    // 1.0f for the whole cell, out = dry by COPY. dry·1 + wet·0 is not bitwise
    // (signed zeros, NaN/Inf wet). The grains above still ran, so the engine
    // stays warm and the draw sequence is unchanged.
    if (wetGain.settledAt (0.0f) && dryGain.settledAt (1.0f) && outGainRamp.settledAt (1.0f))
    {
        for (int i = 0; i < len; ++i)
        {
            const float xl = dryL[i];   // read before write: in/out may alias
            const float xr = dryR[i];
            outL[i] = xl;                // mono→mono: dryL IS the input channel
            if (outR != nullptr)
                outR[i] = xr;
        }
        return;
    }

    for (int i = 0; i < len; ++i)
    {
        const int c = cellPos + i;

        const float xl = dryL[i];       // read before write: in/out may alias
        const float xr = dryR[i];

        float wl = 0.0f, wr = 0.0f;
        for (int s = 0; s < kMaxStreams; ++s)
        {
            wl += bus[s][0][i] * streamGainL[s].at (c);
            wr += bus[s][1][i] * streamGainR[s].at (c);
        }

        const float dg = dryGain.at (c);
        const float wg = wetGain.at (c);
        const float og = outGainRamp.value (c);
        const float yl = (xl * dg + wl * wg) * og;
        const float yr = (xr * dg + wr * wg) * og;

        if (numOut == 1)
        {
            outL[i] = 0.5f * (yl + yr);   // mono out = (L + R) / 2 of the final mix
        }
        else
        {
            outL[i] = yl;
            outR[i] = yr;
        }
    }
}
} // namespace ogran
