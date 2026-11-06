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
#include <limits>

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
    tracker.prepare (fs);   // 2.5a: virtual 120 bpm until the first host block

    for (auto& ch : fbDelay)
        std::fill (std::begin (ch), std::end (ch), 0.0f);
    chain.prepare (fs);   // 2.4: lengths, AGC coefficients, state cleared (fixed arrays — no allocation)
    chainLowHz = chainHighHz = -1.0f;
    for (auto& ch : wet)
        std::fill (std::begin (ch), std::end (ch), 0.0f);
    for (auto& ch : tap)
        std::fill (std::begin (ch), std::end (ch), 0.0f);
    for (auto& sb : bus)
        for (auto& ch : sb)
            std::fill (std::begin (ch), std::end (ch), 0.0f);

    absCount = 0;
    gp.latch (ptrs);
    if (gp.syncMode == 1)
        applyTempoEffective();   // 2.5a: ramps snap to the synced values (virtual 120 bpm until the first block)
    // Freeze: prepare starts LIVE (a saved freeze On engages at the first tick —
    // its rising edge — with whatever has been captured by then).
    frozen = false;
    freezePending = false;
    freezeStopTick = 0;
    fz = {};
    fz.xfadeLen = std::max (1, static_cast<int> (std::lround (kFreezeXfadeS * fs)));
    fz.slopeLen = std::max (1, static_cast<int> (std::lround (kFreezeSlopeS * fs)));
    // The deferral covers the 5 ms kill fade in whole cells (+1 for the float
    // killStep's last decrement): 9 cells @ 48 k, 8 @ 44.1 k, 31 @ 192 k.
    freezeDeferCells = rampCellsFor (fs, kKillFadeMs) + 1;
    spanSamples = static_cast<double> (gp.bufferLengthS) * fs;
    copyCustomTable (true);
    for (auto& a : activeSets)
        a.invalidate();
    updateScaleState();
    snapRampsToParams();
    activeSeed = pendingSeed;

    // 2.5a sources: LFO phases ramp from the latched phase, accumulators at 0,
    // the follower empty, macros snapped (no fade at load).
    for (int n = 0; n < kLfos; ++n)
    {
        lfos[n].prepare (fs, rampCellsFor (fs, kModRampMs), gp.lfo[n].phaseDeg);
        lfoRateHz[n]  = static_cast<double> (gp.lfo[n].rateHz);
        lfoRateOff[n] = 0.0f;
        macroRamp[n].reset (gp.macro[n]);
    }
    env.prepare (fs, gp.envAttackMs, gp.envReleaseMs, gp.envGainDb);
    srcVals = {};

    // 2.5a matrix: slots snap to the latched routes (no fade at load).
    matrix.prepare (rampCellsFor (fs, kModRampMs), gp);
    std::fill (std::begin (rawBase), std::end (rawBase), 0.0f);
    for (auto& r : streamRouted) r = false;
    inGainNow  = inGainRamp.current();   inGainMod.reset (inGainNow);
    outGainNow = outGainRamp.current();  outGainMod.reset (outGainNow);
    fbKnobNow  = fbKnobRamp.current();   fbKnobMod.reset (fbKnobNow);
    fbDiffNow  = fbDiffRamp.current();   fbDiffMod.reset (fbDiffNow);
    fbSatDNow  = fbSatDRamp.current();   fbSatDMod.reset (fbSatDNow);
    useInMod = useOutMod = useFbKnobMod = useFbDiffMod = useFbSatMod = false;
    for (auto& t : srcTelemetry) t.store (0.0f, std::memory_order_relaxed);

    // 2.4 reverb: atomics from the latch, then prepare (allocates its lines —
    // message thread). reverb_mix 0 → the unit never runs (bitwise, R35).
    writeReverbAtomics();
    reverb.prepare (fs, kCell);

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

    float gOut[kMaxStreams] {}, aud[kMaxStreams] {}, gFb[kMaxStreams] {};
    computeStreamTargets (gOut, aud, gFb);
    for (int s = 0; s < kMaxStreams; ++s)
    {
        const auto& st = gp.s[s];
        gFbRamp[s].reset (gFb[s]);
        fbEnRamp[s].reset (st.enable ? 1.0f : 0.0f);
        levelRamp[s].reset (levelDbToGain (st.levelDb));
        panRamp[s].reset (st.pan);
        gOutRamp[s].reset (gOut[s]);
        audRamp[s].reset (aud[s]);

        const auto pg = panGains (panRamp[s].current());
        const float g = gOutRamp[s].current() * levelRamp[s].current() * audRamp[s].current();
        streamGainL[s].reset (g * pg.a);
        streamGainR[s].reset (g * pg.b);
        levelNow[s] = levelRamp[s].current();
    }

    // 2.4 feedback network.
    fbKnobRamp.reset (kFbKnob * gp.feedback);
    fbDiffRamp.reset (gp.fbDiff);
    fbSatDRamp.reset (1.0f + kSatSpan * gp.fbSat);
    fbLowLogRamp.reset (std::log2 (gp.fbLowCutHz));
    fbHighLogRamp.reset (std::log2 (gp.fbHighCutHz));
    updateChainCutoffs();
    pushFbTapGains (true);
}

/** Tap gain per stream from the CURRENT ramp values (R29 / R31): no pan, no
    g_out; muted / un-soloed / disabled streams feed 0 and leave the Σ. */
void Engine::pushFbTapGains (bool snap) noexcept
{
    float sum = 0.0f;
    for (int s = 0; s < kMaxStreams; ++s)
        sum += levelNow[s] * audRamp[s].current() * fbEnRamp[s].current();
    const float norm = 1.0f / std::max (1.0f, sum);
    for (int s = 0; s < kMaxStreams; ++s)
    {
        const float g = gFbRamp[s].current() * levelNow[s] * audRamp[s].current() * fbEnRamp[s].current() * norm;
        if (snap) fbTapCell[s].reset (g);
        else      fbTapCell[s].push (g);
    }
}

/** D11: the reverb's plugin-owned atomics, written at grid ticks only (so a
    target change is first seen by the piece that starts the cell — block-size
    invariant, R35). Fixed internals: enabled 1, predelay 20 ms, mod 0.2,
    shimmer 0. */
void Engine::writeReverbAtomics() noexcept
{
    // 2.5a (R54, closes the 2.4 carry): routed reverb dests write the modulated
    // value of this cell; the unit smooths size (100 ms) and gates mix <= 0.001.
    float size = gp.reverbSize, damp = gp.reverbDamp, mixV = gp.reverbMix;
    if (matrix.routed (ModDest::RevSize)) size = matrix.apply (ModDest::RevSize, matrix.rawNow (ModDest::RevSize), matrix.offset (ModDest::RevSize)) / 100.0f;
    if (matrix.routed (ModDest::RevDamp)) damp = matrix.apply (ModDest::RevDamp, matrix.rawNow (ModDest::RevDamp), matrix.offset (ModDest::RevDamp)) / 100.0f;
    if (matrix.routed (ModDest::RevMix))  mixV = matrix.apply (ModDest::RevMix,  matrix.rawNow (ModDest::RevMix),  matrix.offset (ModDest::RevMix))  / 100.0f;
    revSize.store (size, std::memory_order_relaxed);
    revDamp.store (damp, std::memory_order_relaxed);
    revMix.store (mixV, std::memory_order_relaxed);
    traceAt (ModDest::RevSize, absCount, size);
    traceAt (ModDest::RevDamp, absCount, damp);
    traceAt (ModDest::RevMix,  absCount, mixV);
}

/** fb HP / LP coefficients from the ramped log-cutoffs (tan only here, and only
    when a cutoff moved). */
void Engine::updateChainCutoffs() noexcept
{
    float lo = std::exp2 (fbLowLogRamp.current()), hi = std::exp2 (fbHighLogRamp.current());
    if (matrix.routed (ModDest::FbLowCut))  lo = matrix.apply (ModDest::FbLowCut,  lo, matrix.offset (ModDest::FbLowCut));
    if (matrix.routed (ModDest::FbHighCut)) hi = matrix.apply (ModDest::FbHighCut, hi, matrix.offset (ModDest::FbHighCut));
    traceAt (ModDest::FbLowCut, absCount, lo);
    traceAt (ModDest::FbHighCut, absCount, hi);
    if (juce::exactlyEqual (lo, chainLowHz) && juce::exactlyEqual (hi, chainHighHz))
        return;
    chainLowHz = lo;
    chainHighHz = hi;
    chain.setCutoffs (static_cast<double> (lo), static_cast<double> (hi));
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
void Engine::computeStreamTargets (float (&gOut)[kMaxStreams], float (&audible)[kMaxStreams], float (&gFb)[kMaxStreams]) noexcept
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

        // 2.4 tap law (R29, signed off): 1/(S1 + 1), S1 pre-spread.
        const double s1 = fbTapS1 (n, q, win, bank);
        gFb[s] = fbTapGain (s1);
       #if OGRAN_TEST_HOOKS
        if (hooks.fbArchLaw)
            gFb[s] = fbTapGainArch (s1);
       #endif

        audible[s] = (! st.mute && (! anySolo || st.solo)) ? 1.0f : 0.0f;
    }
}

//==============================================================================
// Audio thread
//==============================================================================
void Engine::process (const float* const* in, int numIn, float* const* out, int numOut,
                      int numSamples, std::uint64_t seed, const TransportSnapshot& transport) noexcept
{
    if (! prepared || numSamples <= 0 || numIn < 1 || numOut < 1 || in == nullptr || out == nullptr)
        return;

    // 2.5a: the host transport, once per host block (R39 held anchor, R42 rules).
   #if OGRAN_TEST_HOOKS
    tracker.perBlockOrigin = hooks.perBlockOrigin;
   #endif
    tracker.update (transport, absCount);
   #if OGRAN_TEST_HOOKS
    diag.tempoReanchors = tracker.getReanchors();
    diag.tempoRelocks   = tracker.getRelocks();
   #endif

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

        // 3. Write before read. R23: grains anchor in the RING domain — W_piece
        //    is the head before this piece's write. It equals absCount until the
        //    first freeze; after one, the ring lags absCount by whole cells.
        const std::int64_t wPiece = ring.getWriteCount();
        const ExactRamp& inGain = useInMod ? inGainMod : inGainRamp;   // 2.5a: routed input_gain (R47)
        CaptureWriter::write (ring, inL + done, inR + done, len, cellPos, inGain, fbDelay, fz);
        jassert (ring.getWriteCount() == wPiece + (frozen ? 0 : len));

        // 2.5a envelope follower on the conditioned input (post input_gain,
        // pre-ring; runs while frozen). Never touches the audio path.
        env.process (inL + done, inR + done, len, cellPos, [&inGain] (int c) noexcept { return inGain.value (c); });
       #if OGRAN_TEST_HOOKS
        if (hooks.injectEnvNanAt >= 0 && absCount + len > hooks.injectEnvNanAt)
        {
            env.injectForTest (std::numeric_limits<float>::quiet_NaN());
            hooks.injectEnvNanAt = -1;
        }
       #endif
        if (env.scrub())
        {
           #if OGRAN_TEST_HOOKS
            ++diag.envScrubs;
           #endif
        }

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
            spawn (o, absCount, wPiece);
        }
        if (seg < len)
            renderSegment (seg, len);

        // 6 + 8. Stream mix + feedback tap, reverb, knee, dry/wet, output gain.
        mixOut (inL + done, inR + done, outL + done, outR != nullptr ? outR + done : nullptr, numOut, len, cellPos);

        // 7. Feedback chain over the tap → fbDelay (z^-32: step 3 of the pass
        //    one cell later reads slot c). Runs every pass, frozen included.
       #if OGRAN_TEST_HOOKS
        if (hooks.tapProbe != nullptr)
            for (int i = 0; i < len; ++i)
                if (absCount + i < hooks.tapProbeLen)
                {
                    hooks.tapProbe[2 * (absCount + i)]     = tap[0][i];
                    hooks.tapProbe[2 * (absCount + i) + 1] = tap[1][i];
                }
        {
            auto& inj = hooks.injectFbNan;
            if (inj.atSample >= 0 && ! inj.done && absCount + len > inj.atSample)
            {
                chain.testInjectAt  = static_cast<int> (std::max<std::int64_t> (0, inj.atSample - absCount));
                chain.testInjectNan = ! inj.reset;
                inj.done = true;
            }
        }
       #endif
        chain.process (tap[0], tap[1], len, cellPos, useFbKnobMod ? fbKnobMod : fbKnobRamp,
                       useFbDiffMod ? fbDiffMod : fbDiffRamp, useFbSatMod ? fbSatDMod : fbSatDRamp);
       #if OGRAN_TEST_HOOKS
        chain.testInjectAt = -1;
        diag.fbScrubs = chain.scrubs;
       #endif
        for (int i = 0; i < len; ++i)
        {
            fbDelay[0][cellPos + i] = tap[0][i];
            fbDelay[1][cellPos + i] = tap[1][i];
        }

        absCount += len;
        done     += len;
    }
}

void Engine::gridTick() noexcept
{
    activeSeed = pendingSeed;
    gp.latch (ptrs);
    spanSamples = static_cast<double> (gp.bufferLengthS) * fs;   // spawn-latched destination (no ramp)
    handleFreezeEdges();

    // 2.5a: the modulation sources of this cell (sampled once, at the tick),
    // then the matrix (spawn-latched + tick destinations overwrite the latch).
    tickSources();
    matrixTick();

    // 2.5a: Tempo mode reads the EFFECTIVE density / size (quotas, coherence,
    // the spawn size and the clock all see the synced values, R45).
    const bool tempoMode = gp.syncMode == 1;
    if (tempoMode)
        applyTempoEffective();

    // All four streams (2.2 removes the 2.1 P5 mask). Each keeps its R4 epoch and
    // rising-edge reset; a Free ↔ Tempo switch folds the epoch the same way.
    for (int s = 0; s < kMaxStreams; ++s)
    {
        scheds[s].setTempo (tempoMode);
        scheds[s].setEnabled (gp.s[s].enable);
    }

    // P11 custom-table copy (version-gated, non-blocking), then active sets.
    copyCustomTable (false);
    updateScaleState();

    inGainRamp.setTarget (dbToGain (gp.inputGainDb), rampCells);
    inGainRamp.tick();
    outGainRamp.setTarget (dbToGain (gp.outputGainDb), rampCells);
    outGainRamp.tick();
    mixRamp.setTarget (gp.mix, rampCells);
    mixRamp.tick();

    // 2.5a R47: routed ramped destinations are modulated AFTER the base ramp,
    // in that ramp's domain; per sample the substitute lerps prev → cur.
    {
        using namespace ModDest;
        const bool ri = matrix.routed (InputGain), ro = matrix.routed (OutputGain);
        const float gi = ri ? dbToGain (matrix.apply (InputGain, ModMap::gainToRaw (inGainRamp.current()), matrix.offset (InputGain)))
                            : inGainRamp.current();
        const float go = ro ? dbToGain (matrix.apply (OutputGain, ModMap::gainToRaw (outGainRamp.current()), matrix.offset (OutputGain)))
                            : outGainRamp.current();
        inGainMod.setCell (inGainNow, gi);   inGainNow = gi;
        outGainMod.setCell (outGainNow, go); outGainNow = go;
        useInMod  = ri || matrix.routedPrev (InputGain);
        useOutMod = ro || matrix.routedPrev (OutputGain);
        traceAt (InputGain, absCount, gi);
        traceAt (OutputGain, absCount, go);
    }
    float mixNow = mixRamp.current();
    if (matrix.routed (ModDest::Mix))
        mixNow = matrix.apply (ModDest::Mix, mixNow * 100.0f, matrix.offset (ModDest::Mix)) / 100.0f;
    traceAt (ModDest::Mix, absCount, mixNow);

    const auto dw = mixGains (mixNow);   // snapped ends: exactly (1, 0) at mix 0
    dryGain.push (dw.a);
    wetGain.push (dw.b);

    // Quotas → coherence → g_out (20 ms); audibility, level, pan (10 ms, P12).
    float gOut[kMaxStreams] {}, aud[kMaxStreams] {}, gFb[kMaxStreams] {};
    computeStreamTargets (gOut, aud, gFb);
    for (int s = 0; s < kMaxStreams; ++s)
    {
        const auto& st = gp.s[s];
        gFbRamp[s].setTarget (gFb[s], rampCells);
        gFbRamp[s].tick();
        fbEnRamp[s].setTarget (st.enable ? 1.0f : 0.0f, mixerRampCells);
        fbEnRamp[s].tick();
        levelRamp[s].setTarget (levelDbToGain (st.levelDb), mixerRampCells);
        levelRamp[s].tick();
        panRamp[s].setTarget (st.pan, mixerRampCells);
        panRamp[s].tick();
        audRamp[s].setTarget (aud[s], mixerRampCells);
        audRamp[s].tick();
        gOutRamp[s].setTarget (gOut[s], rampCells);
        gOutRamp[s].tick();

        // 2.5a R47: routed level / pan modulate the ramped value.
        float lv = levelRamp[s].current(), pn = panRamp[s].current();
        const int dl = ModDest::stream (s, ModDest::Level), dp = ModDest::stream (s, ModDest::Pan);
        if (matrix.routed (dl)) lv = levelDbToGain (matrix.apply (dl, ModMap::levelToRaw (lv), matrix.offset (dl)));
        if (matrix.routed (dp)) pn = matrix.apply (dp, pn * 100.0f, matrix.offset (dp)) / 100.0f;
        levelNow[s] = lv;
        traceAt (dl, absCount, lv);
        traceAt (dp, absCount, pn);

        // (g_out · level) · audible: audible == 1.0f leaves the 2.1 product bitwise.
        const auto pg = panGains (pn);
        const float g = gOutRamp[s].current() * lv * audRamp[s].current();
        streamGainL[s].push (g * pg.a);
        streamGainR[s].push (g * pg.b);
    }

    // 2.4 reverb atomics (base values; 2.5's matrix modulates them here).
    writeReverbAtomics();

    // 2.4 feedback network: tap gains, chain controls, cutoffs.
    pushFbTapGains (false);
    fbKnobRamp.setTarget (kFbKnob * gp.feedback, rampCells);
    fbKnobRamp.tick();
    fbDiffRamp.setTarget (gp.fbDiff, rampCells);
    fbDiffRamp.tick();
    fbSatDRamp.setTarget (1.0f + kSatSpan * gp.fbSat, rampCells);
    fbSatDRamp.tick();
    {
        using namespace ModDest;
        const bool rk = matrix.routed (Feedback), rd = matrix.routed (FbDiff), rs = matrix.routed (FbSat);
        const float k = rk ? ModMap::fbKnobFromRaw (matrix.apply (Feedback, ModMap::fbKnobToRaw (fbKnobRamp.current()), matrix.offset (Feedback)))
                           : fbKnobRamp.current();
        const float df = rd ? ModMap::pctFromRaw (matrix.apply (FbDiff, fbDiffRamp.current() * 100.0f, matrix.offset (FbDiff)))
                            : fbDiffRamp.current();
        const float sd = rs ? ModMap::satDFromRaw (matrix.apply (FbSat, ModMap::satDToRaw (fbSatDRamp.current()), matrix.offset (FbSat)))
                            : fbSatDRamp.current();
        fbKnobMod.setCell (fbKnobNow, k);  fbKnobNow = k;
        fbDiffMod.setCell (fbDiffNow, df); fbDiffNow = df;
        fbSatDMod.setCell (fbSatDNow, sd); fbSatDNow = sd;
        useFbKnobMod = rk || matrix.routedPrev (Feedback);
        useFbDiffMod = rd || matrix.routedPrev (FbDiff);
        useFbSatMod  = rs || matrix.routedPrev (FbSat);
        traceAt (Feedback, absCount, k);
        traceAt (FbDiff, absCount, df);
        traceAt (FbSat, absCount, sd);
    }
    fbLowLogRamp.setTarget (std::log2 (gp.fbLowCutHz), rampCells);
    fbLowLogRamp.tick();
    fbHighLogRamp.setTarget (std::log2 (gp.fbHighCutHz), rampCells);
    fbHighLogRamp.tick();
    updateChainCutoffs();

   #if OGRAN_TEST_HOOKS
    if (hooks.snapshotChain)
    {
        auto& sn = chainSnaps[static_cast<size_t> (chainSnapCount % static_cast<std::uint64_t> (kChainSnaps))];
        sn.tick  = absCount;
        sn.hpIc1 = chain.hpIc1[0]; sn.hpIc2 = chain.hpIc2[0];
        sn.lpIc1 = chain.lpIc1[0]; sn.lpIc2 = chain.lpIc2[0];
        sn.env   = chain.env;
        ++chainSnapCount;
    }
   #endif
}

/** Freeze edges from the LATCHED Bool at absolute ticks (block-size invariant;
    never a listener write).

    Rising edge — DEFERRED head-stop (2.4 execution deviation, see SUMMARY-2.4):
    the stop W0 is latched as the head freezeDeferCells cells AHEAD (the ring
    keeps writing until then), and the R25 forced release is computed against
    that W0. The deferral covers the 5 ms kill fade, so every crosser is silent
    before the head stops and none ever reaches it: R25's reflect (value-
    continuous, but a slope REVERSAL — a 2·s' kink on tonal material, κ ≈ 24x)
    stays only as a safety net. Spawns inside the window already use the frozen
    geometry against W0 (+ a head-lead clamp, Spawner::frozenDelay). The stop
    lands on a tick, so the whole edge stays block-size invariant.
    Falling edge: a 10 ms mirror crossfade (R26); during the deferral it just
    cancels the pending stop. A rising edge inside a release fade restarts the
    deferral; the fade ends at the stop (the push stops, nothing is blended). */
void Engine::handleFreezeEdges() noexcept
{
    const bool engaged = frozen || freezePending;
    if (gp.freeze && ! engaged)
    {
        bool deferStop = true;
       #if OGRAN_TEST_HOOKS
        deferStop = ! hooks.freezeNoDefer;
       #endif
        const std::int64_t lead = deferStop ? static_cast<std::int64_t> (freezeDeferCells) * kCell : 0;
        fz.W0           = ring.getWriteCount() + lead;
        fz.heldCaptured = std::clamp<std::int64_t> (fz.W0 - Ring::margin (kMaxLevel) - 1, 0, ringSamples);
        freezePending   = true;
        freezeStopTick  = absCount + lead;
       #if OGRAN_TEST_HOOKS
        ++diag.freezeEngages;
        const auto& all = pool.getGrains();
        for (int i = 0; i < kPoolLive + kPoolFade; ++i)
        {
            engageSnap[static_cast<size_t> (i)]   = all[static_cast<size_t> (i)];
            engageActive[static_cast<size_t> (i)] = pool.isActive (i);
        }
        engageW0 = fz.W0;
       #endif
        forcedReleaseScan();
    }
    else if (! gp.freeze && engaged)
    {
        if (frozen)
        {
            frozen      = false;
            fz.frozen   = false;
            fz.xfading  = true;
            fz.xfadePos = 0;
            fz.heldSpan = std::min (static_cast<std::int64_t> (spanSamples), fz.heldCaptured);
           #if OGRAN_TEST_HOOKS
            fz.spliceOld = hooks.releaseSpliceOld;
           #endif
        }
        freezePending = false;   // released inside the deferral: the head never stopped
       #if OGRAN_TEST_HOOKS
        ++diag.freezeReleases;
       #endif
    }

    // The deferred stop, on its tick.
    if (freezePending && absCount >= freezeStopTick)
    {
        jassert (ring.getWriteCount() == fz.W0);
        freezePending = false;
        frozen        = true;
        fz.frozen     = true;
        fz.xfading    = false;
    }
}

/** R25: a live forward grain whose remaining reach crosses the stopped head
    (δ − r·m < margin(level)) gets the 2.2 5 ms release. Reverse grains never
    cross (δ grows by r per sample); fade slots are already fading. */
void Engine::forcedReleaseScan() noexcept
{
    const auto& all = pool.getGrains();
    for (int i = 0; i < kPoolLive; ++i)
    {
        if (! pool.isActive (i))
            continue;
        const Grain& g = all[static_cast<size_t> (i)];
        if (g.dir < 0 || g.killStep > 0.0f || spawner.isPendingKill (i))
            continue;
        const double delta = static_cast<double> (fz.W0) - g.readAbs;
        const double rem   = g.len - static_cast<double> (g.age) - g.frac;
        if (delta - g.rate * rem < static_cast<double> (Ring::margin (g.level)))
        {
            spawner.requestKill (i);
           #if OGRAN_TEST_HOOKS
            ++diag.forcedReleases;
           #endif
        }
    }
}

/** P51: LFO 1–4, the envelope follower and the macros, sampled ONCE per cell
    at the tick. The LFO rate (and its matrix offset) come from the PREVIOUS
    cell (R50.5: no same-cell source cycle). Pitch / Onset stay 0 (2.5b). */
void Engine::tickSources() noexcept
{
    const bool tempoMode = gp.syncMode == 1;
    int forceMode = 0;
   #if OGRAN_TEST_HOOKS
    forceMode = hooks.forceLfoMode;
   #endif
    for (int n = 0; n < kLfos; ++n)
        srcVals.v[ModSrc::Lfo1 + n] = lfos[n].tick (n, gp.lfo[n], tempoMode, tracker, absCount, lfoRateOff[n], lfoRateHz[n],
                                                    activeSeed, forceMode);
    env.setParams (gp.envAttackMs, gp.envReleaseMs, gp.envGainDb);
    srcVals.v[ModSrc::Env]   = env.source();
    srcVals.v[ModSrc::Pitch] = 0.0f;
    srcVals.v[ModSrc::Onset] = 0.0f;
    for (int n = 0; n < kMacros; ++n)
    {
        macroRamp[n].setTarget (gp.macro[n], rampCells);
        macroRamp[n].tick();
        srcVals.v[ModSrc::Macro1 + n] = macroRamp[n].current();
    }
    for (int k = 1; k < kModSources; ++k)
        srcTelemetry[static_cast<size_t> (k - 1)].store (srcVals.v[k], std::memory_order_relaxed);
}

/** P52–P55: the matrix of this cell. Spawn-latched and tick destinations
    overwrite the latch with the cell's modulated value (tick consumers —
    quotas, R5 coherence, the Free interval, the scale window — read it; the
    spawn re-evaluates at its own sample, P54). Ramped destinations are applied
    after their base ramps in gridTick (P53). Unrouted: nothing is touched. */
void Engine::matrixTick() noexcept
{
   #if OGRAN_TEST_HOOKS
    matrix.noResidual    = hooks.noResidual;
    matrix.fullRoundTrip = hooks.fullRoundTrip;
   #endif
    const int changes = matrix.tick (gp, srcVals);
   #if OGRAN_TEST_HOOKS
    diag.routeChanges += changes;
    diag.routedDests = std::max<std::int64_t> (diag.routedDests, matrix.routedCount());
   #else
    juce::ignoreUnused (changes);
   #endif

    // LFO rates of the NEXT cell (R50.5: no same-cell source cycle).
    for (int n = 0; n < kLfos; ++n)
    {
        const int d = ModDest::LfoRate1 + n;
        if (matrix.routed (d))
        {
            lfoRateOff[n] = matrix.offset (d);
            lfoRateHz[n]  = static_cast<double> (matrix.apply (d, gp.lfo[n].rateHz, lfoRateOff[n]));
        }
        else
        {
            lfoRateOff[n] = 0.0f;
            lfoRateHz[n]  = static_cast<double> (gp.lfo[n].rateHz);
        }
        traceAt (d, absCount, lfoRateHz[n]);
    }

    const bool tempoMode = gp.syncMode == 1;
    for (int s = 0; s < kMaxStreams; ++s)
    {
        streamRouted[s] = false;
        for (int f = ModDest::Position; f < ModDest::NumFields; ++f)
        {
            const int d = ModDest::stream (s, f);
            if (! matrix.routed (d))
                continue;
            streamRouted[s] = true;
            rawBase[d] = matrix.rawNow (d);
            if (tempoMode && (f == ModDest::Size || f == ModDest::Density))
                continue;   // Tempo: the offset scales the synced value (applyTempoEffective, P50)
            ModMap::streamFieldFromRaw (gp.s[s], f, matrix.apply (d, rawBase[d], matrix.offset (d)));
        }
    }
    if (matrix.routed (ModDest::BufferLength))
    {
        rawBase[ModDest::BufferLength] = matrix.rawNow (ModDest::BufferLength);
        gp.bufferLengthS = std::clamp (matrix.apply (ModDest::BufferLength, rawBase[ModDest::BufferLength],
                                                     matrix.offset (ModDest::BufferLength)), 1.0f, 30.0f);
        spanSamples = static_cast<double> (gp.bufferLengthS) * fs;
    }

    matrix.writeTelemetry (destTelemetry);
}

/** P50 (R45, signed off): per stream, the synced density as a power-of-two
    multiple of the division clamped to [0.5, 200] g/s, and the synced size
    clamped to [5, 2000] ms. Overwrites the latched gp fields (re-latched every
    tick), so every tick consumer and the spawn read the effective values. */
void Engine::applyTempoEffective() noexcept
{
    const TempoAnchor& A = tracker.anchor();
    const double beatsPerS = A.pps * fs;
    for (int s = 0; s < kMaxStreams; ++s)
    {
        auto& st = gp.s[s];
        const double divBeats = tempoDivBeats (st.densitySync, A.barLen);
        const double d0 = beatsPerS / divBeats;
        const double offD = static_cast<double> (matrix.offset (ModDest::stream (s, ModDest::Density)));
        const double eLo = std::ceil (std::log2 (kDensityMin / d0)), eHi = std::floor (std::log2 (kDensityMax / d0));
        const double e = std::clamp (std::floor (4.0 * offD + 0.5), eLo, eHi);
        tempoDiv[s] = divBeats * std::exp2 (-e);
        st.density  = static_cast<float> (d0 * std::exp2 (e));

        const double offS = static_cast<double> (matrix.offset (ModDest::stream (s, ModDest::Size)));
        tempoSizeBaseMs[s] = tempoDivBeats (st.sizeSync, A.barLen) * 60000.0 / A.bpm;
        const double ms = tempoSizeBaseMs[s] * std::exp2 (4.0 * offS);
        st.sizeMs = static_cast<float> (std::clamp (ms, kMinSizeMs, kMaxSizeMs));
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
        if (scheds[s].isTempo())
        {
            StreamScheduler::TempoGrid g;
            g.anchor      = &tracker.anchor();
            g.div         = tempoDiv[s];
            g.relockEpoch = tracker.getRelockEpoch();
           #if OGRAN_TEST_HOOKS
            g.rederive    = hooks.ceilPerTick && (absCount % kCell) == 0;
           #endif
            counts[s] = scheds[s].processTempo (len, absCount, s, activeSeed, g, st.jitter, streamOnsets[s]);
           #if OGRAN_TEST_HOOKS
            diag.tempoOnsets += counts[s];
           #endif
        }
        else
        {
            const double interval = fs / static_cast<double> (st.density);   // Free timing (GrainScheduler, verbatim)
            counts[s] = scheds[s].process (len, s, activeSeed, interval, st.jitter, streamOnsets[s]);
        }
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

void Engine::spawn (const StreamOnset& o, std::int64_t pieceStart, std::int64_t wPiece) noexcept
{
    bool forceLevel0 = false;
   #if OGRAN_TEST_HOOKS
    forceLevel0 = hooks.forceLevel0;
   #endif

    // Draw before obtain. While frozen: the R24 / R27 geometry against the
    // captured span (span_frz re-taken at every grid point from the latch).
    // Inside the deferral the head still moves toward W0: headLead = W0 − W_on(live).
    FrozenGeom fzg;
    const bool frozenGeom = frozen || freezePending;
    if (frozenGeom)
    {
        fzg.on           = true;
        fzg.heldCaptured = static_cast<double> (fz.heldCaptured);
        fzg.spanFrz      = std::min (spanSamples, fzg.heldCaptured);
        if (freezePending)
            fzg.headLead = static_cast<double> (std::max<std::int64_t> (0, fz.W0 - (wPiece + o.offset + 1)));
       #if OGRAN_TEST_HOOKS
        fzg.noMinSpray   = hooks.freezeNoMinSpray;
       #endif
    }
    // 2.5a P54 (R48): routed spawn-latched fields are evaluated AT the spawn
    // sample — the offset lerped prev → cur at the onset's cell position — into
    // a local copy; unrouted fields are copied bitwise. No route: the 2.4 call.
    const GridParams::Stream* sp = &gp.s[o.stream];
    GridParams::Stream local;
    double span = spanSamples;
    const bool bufRouted = matrix.routed (ModDest::BufferLength);
    if (streamRouted[o.stream] || bufRouted)
    {
        local = gp.s[o.stream];
        const double at = static_cast<double> (absCount % kCell) + static_cast<double> (o.offset);
        const bool tempoMode = gp.syncMode == 1;
        for (int f = ModDest::Position; f < ModDest::NumFields && streamRouted[o.stream]; ++f)
        {
            const int d = ModDest::stream (o.stream, f);
            if (! matrix.routed (d))
                continue;
            const float off = matrix.offsetAt (d, at);
            if (tempoMode && f == ModDest::Density)
                continue;
            if (tempoMode && f == ModDest::Size)
            {
                local.sizeMs = static_cast<float> (std::clamp (tempoSizeBaseMs[o.stream] * std::exp2 (4.0 * static_cast<double> (off)),
                                                               kMinSizeMs, kMaxSizeMs));
                continue;
            }
            ModMap::streamFieldFromRaw (local, f, matrix.apply (d, rawBase[d], off));
        }
        if (bufRouted)
        {
            const float bl = std::clamp (matrix.apply (ModDest::BufferLength, rawBase[ModDest::BufferLength],
                                                       matrix.offsetAt (ModDest::BufferLength, at)), 1.0f, 30.0f);
            span = static_cast<double> (bl) * fs;
        }
        sp = &local;
    }
   #if OGRAN_TEST_HOOKS
    {
        const int td = hooks.traceDest.dest;
        if (td >= ModDest::StreamBase && td < ModDest::Feedback && (td - ModDest::StreamBase) / ModDest::StreamStride == o.stream
            && (td - ModDest::StreamBase) % ModDest::StreamStride >= ModDest::Position)   // level / pan are ramped (traced at ticks)
            traceAt (td, pieceStart + o.offset, ModMap::streamFieldValue (*sp, (td - ModDest::StreamBase) % ModDest::StreamStride),
                     matrix.offsetAt (td, static_cast<double> (absCount % kCell) + static_cast<double> (o.offset)));
        if (td == ModDest::BufferLength)
            traceAt (td, pieceStart + o.offset, span / fs);
    }
   #endif
    if (frozenGeom)
        fzg.spanFrz = std::min (span, fzg.heldCaptured);
    const GrainPlan gpl = Spawner::plan (*sp, span, fs, ringSamples,
                                         activeSeed, o.stream, o.globalIndex, forceLevel0, pitchSel[o.stream], fzg);
   #if OGRAN_TEST_HOOKS
    ++diag.planCalls[o.stream];
    if (hooks.snapshotSteals)
        std::copy (pool.getGrains().begin(), pool.getGrains().begin() + kPoolLive, stealSnap.begin());
    AcquirePolicy policy;
    policy.nativeProgress = hooks.nativeProgress;
    policy.noQuotaCeil    = hooks.noQuotaCeil;
    const auto acq = hooks.nativeAcquire ? nativeAcquireForTest()
                                         : spawner.acquire (pool, gp.grainLimit, quotas, o.stream, policy);
   #else
    const auto acq = spawner.acquire (pool, gp.grainLimit, quotas, o.stream);
   #endif

    // R23: absolute onset (telemetry, epochs) vs the ring-domain head the grain
    // anchors to. Live: the head after the onset sample is written. Frozen: the
    // stopped head (every frozen spawn sees the same W).
    const std::int64_t wOn = frozen ? wPiece : freezePending ? fz.W0 : wPiece + o.offset + 1;
    Spawner::fill (*acq.grain, gpl, o.stream, o.frac, wOn);
    acq.grain->frozenSpawn = frozenGeom;
   #if OGRAN_TEST_HOOKS
    const std::int64_t onsetSample = pieceStart + o.offset;
   #else
    juce::ignoreUnused (pieceStart);
   #endif

   #if OGRAN_TEST_HOOKS
    {
        Grain& g = *acq.grain;
        // H3: every FX STATE field must be at its Grain {} default right after fill.
        const Grain d {};
        if (! juce::exactlyEqual (g.svfIc1[0], d.svfIc1[0]) || ! juce::exactlyEqual (g.svfIc1[1], d.svfIc1[1])
            || ! juce::exactlyEqual (g.svfIc2[0], d.svfIc2[0]) || ! juce::exactlyEqual (g.svfIc2[1], d.svfIc2[1])
            || ! juce::exactlyEqual (g.crushHeld[0], d.crushHeld[0]) || ! juce::exactlyEqual (g.crushHeld[1], d.crushHeld[1])
            || ! juce::exactlyEqual (g.crushHoldPhase, d.crushHoldPhase)
            || ! juce::exactlyEqual (g.ringRe, d.ringRe) || ! juce::exactlyEqual (g.ringIm, d.ringIm))
            ++diag.fxStateNonDefaultAtSpawn;

        if (hooks.fxSharedLatch)
        {
            // CounterRng is stateless: re-planning is side-effect free.
            GridParams::Stream sh = gp.s[o.stream];
            sh.filterRand = 0.0f;
            sh.ringRand   = 0.0f;
            sh.crushProb  = 1.0f;
            const GrainPlan shared = Spawner::plan (sh, spanSamples, fs, ringSamples, activeSeed, o.stream,
                                                    o.globalIndex, forceLevel0, pitchSel[o.stream], fzg);
            Spawner::assignFxLatch (g, shared);
        }
        if (hooks.fxNoReset && acq.slot >= 0 && acq.slot < kPoolLive && retireShadow[acq.slot].valid)
        {
            const auto& sh = retireShadow[acq.slot];
            for (int c = 0; c < 2; ++c)
            {
                g.svfIc1[c] = sh.svfIc1[c];
                g.svfIc2[c] = sh.svfIc2[c];
                g.crushHeld[c] = sh.crushHeld[c];
            }
            g.crushHoldPhase = sh.crushHoldPhase;
            g.ringRe = sh.ringRe;
            g.ringIm = sh.ringIm;
        }
        if (hooks.legacySvfMake)
            g.svfMake = 1.0f;   // P33: the 2.3 LP/HP output (D8) — attribution runs
        if (hooks.snapshotSteals && acq.stolen)
            checkStealState (acq);
    }

    ++diag.spawns;
    if (acq.killed) ++diag.kills;
    if (acq.stolen) ++diag.steals;
    diag.maxAlive = std::max (diag.maxAlive, static_cast<std::int64_t> (acq.aliveAfter));

    auto& rec = diag.log[static_cast<std::size_t> (diag.logCount % static_cast<std::uint64_t> (kSpawnLogSize))];
    rec.onsetSample = onsetSample;
    rec.writeHead   = wOn;
    rec.globalIndex = o.globalIndex;
    rec.stream      = o.stream;
    rec.slot        = acq.slot;
    rec.dir         = gpl.dir;
    rec.level       = gpl.level;
    rec.set         = gpl.set;
    rec.delay       = gpl.delay;
    rec.anchor      = static_cast<double> (wOn) - gpl.delay;
    rec.frac        = o.frac;
    rec.rate        = gpl.rate;
    rec.len         = gpl.len;
    rec.spanSamples = static_cast<float> (span);
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
    rec.fxActive    = gpl.fxActive;
    rec.svfMode     = gpl.svfMode;
    rec.fcHz        = gpl.fcHz;
    rec.ringHz      = gpl.ringHz;
    rec.crushed     = gpl.crushed;
    rec.driveD      = gpl.driveD;
    ++diag.logCount;
   #endif
}

#if OGRAN_TEST_HOOKS
/** H2: the stolen grain (snapshot of acq.slot before acquire) must sit in a fade
    slot with its FX latch AND state bitwise intact. */
void Engine::checkStealState (const Spawner::Acquired& acq) noexcept
{
    const Grain& v = stealSnap[static_cast<size_t> (acq.slot)];
    const auto& all = pool.getGrains();
    for (int k = kPoolLive; k < kPoolLive + kPoolFade; ++k)
    {
        const Grain& f = all[static_cast<size_t> (k)];
        if (! pool.isActive (k) || f.stream != v.stream || f.age != v.age
            || std::memcmp (&f.readAbs, &v.readAbs, sizeof (double)) != 0)
            continue;
        ++diag.stealStateChecked;
        auto same = [] (const void* a, const void* b, size_t n) { return std::memcmp (a, b, n) == 0; };
        const bool ok = f.fxActive == v.fxActive && f.svfMode == v.svfMode
                     && same (&f.svfK, &v.svfK, sizeof (float)) && same (&f.svfA1, &v.svfA1, sizeof (float))
                     && same (&f.svfA2, &v.svfA2, sizeof (float)) && same (&f.svfA3, &v.svfA3, sizeof (float))
                     && same (&f.svfMake, &v.svfMake, sizeof (float))
                     && same (f.svfIc1, v.svfIc1, sizeof (f.svfIc1)) && same (f.svfIc2, v.svfIc2, sizeof (f.svfIc2))
                     && same (&f.driveD, &v.driveD, sizeof (float)) && same (&f.driveNorm, &v.driveNorm, sizeof (float))
                     && same (&f.crushHoldInc, &v.crushHoldInc, sizeof (float)) && same (&f.crushHoldPhase, &v.crushHoldPhase, sizeof (float))
                     && same (&f.crushScale, &v.crushScale, sizeof (float)) && same (f.crushHeld, v.crushHeld, sizeof (f.crushHeld))
                     && same (&f.ringRe, &v.ringRe, sizeof (float)) && same (&f.ringIm, &v.ringIm, sizeof (float))
                     && same (&f.ringRotRe, &v.ringRotRe, sizeof (float)) && same (&f.ringRotIm, &v.ringRotIm, sizeof (float))
                     && same (&f.ringMix, &v.ringMix, sizeof (float));
        if (! ok)
            ++diag.stealStateMismatches;
        return;
    }
}
#endif

void Engine::renderSegment (int from, int to) noexcept
{
    // The ring head at render time (= end of this piece after step 3). Absolute
    // read positions do not depend on it; R1 keeps the delay round-trip exact.
    const std::int64_t head       = ring.getWriteCount();
    const bool         applyKills = spawner.hasPendingKills();
    const Grain* const base       = pool.getGrains().data();
   #if OGRAN_TEST_HOOKS
    const std::int64_t segStart   = absCount + from;
   #endif

    // P18: ONE kernel for every grain — gather → FX → scatter.
    //   gather : the 2.2 per-sample loop (both window branches, statements
    //            verbatim) into L / R / W, stopping at the retire condition;
    //   FX     : one tight loop per ACTIVE stage (flag test hoisted per
    //            segment), every operation per sample (R13);
    //   scatter: bl[from + j] += W[j] · L[j] · gainL — the 2.2 expression.
    // With fxActive == 0 the float expressions and their order are those of
    // 2.2, so the render is bitwise 2.2 (N0). Do not split the W or scatter
    // statements: FP contraction works per statement.
    pool.render ([this, from, to, head, applyKills, base
                 #if OGRAN_TEST_HOOKS
                  , segStart
                 #endif
                 ] (Grain& g, Pool::Fader& fade) noexcept -> bool
    {
        if (applyKills)
            spawner.applyPendingKill (g, base);

        const int slot = static_cast<int> (&g - base);
        juce::ignoreUnused (slot);
        const bool releasing = slot >= kPoolLive || g.killStep > 0.0f;   // fade slot or forced release (R25 diag split)

       #if OGRAN_TEST_HOOKS
        bool forcedRetire = false;
        {
            auto& inj = hooks.injectNan;
            if (inj.slot == slot && ! inj.done && segStart >= inj.atSample && (g.fxActive & kFxSvf) != 0)
            {
                float* const f[4] { &g.svfIc1[0], &g.svfIc1[1], &g.svfIc2[0], &g.svfIc2[1] };
                *f[std::clamp (inj.field, 0, 3)] = inj.value;
                inj.done = true;
            }
            auto& fr = hooks.forceRetire;
            if (fr.slot == slot && ! fr.done && segStart >= fr.atSample && (g.fxActive & kFxSvf) != 0)
            {
                forcedRetire = true;
                fr.done = true;
            }
            if (hooks.fxLiveCoeffs && (g.fxActive & kFxSvf) != 0)
            {
                const auto& st = gp.s[std::clamp (g.stream, 0, kMaxStreams - 1)];
                const auto c = fx::svfLatch (static_cast<double> (st.filterCutoffHz), static_cast<double> (st.filterRes), fs);
                g.svfK = c.k; g.svfA1 = c.a1; g.svfA2 = c.a2; g.svfA3 = c.a3;
                g.svfMake = hooks.legacySvfMake ? 1.0f : c.make;
            }
        }
       #endif

        float L[kCell], R[kCell], W[kCell];
        int  n      = 0;
        bool retire = false;

        // P14: step 0 / skew 0 keeps the closed-form Hann (bitwise 2.1, R12);
        // every other window reads the bank (table + lerp + tilt warp). Latched
        // per grain, so the branch is constant for the whole grain.
        if (g.windowStep == 0 && juce::exactlyEqual (g.tiltT, 0.5f))
        {
            for (int i = from; i < to; ++i)
            {
                float l = 0.0f, r = 0.0f;
                readGrain (g, head, releasing, l, r);

                // Window LAST (after the per-grain FX). killGain and fade are exactly
                // 1.0f for a live, non-killing grain — bitwise no-ops.
                const double phase = (static_cast<double> (g.age) + g.frac) * g.invLen;
                W[n] = hannWindow (phase) * g.killGain * fade.next();
                L[n] = l;
                R[n] = r;
                ++n;

                g.readAbs += g.step;
                ++g.age;
                if (g.killStep > 0.0f)
                    g.killGain = std::max (0.0f, g.killGain - g.killStep);

                if (! ((static_cast<double> (g.age) + g.frac) < g.len && g.killGain > 0.0f))
                {
                    retire = true;
                    break;
                }
            }
        }
        else
        {
            const float* const tab = bank.table (g.windowStep);
            const WindowMorphBank::Tilt tilt { g.tiltT, g.tiltA, g.tiltB };
            for (int i = from; i < to; ++i)
            {
                float l = 0.0f, r = 0.0f;
                readGrain (g, head, releasing, l, r);

                const double phase = (static_cast<double> (g.age) + g.frac) * g.invLen;
                W[n] = WindowMorphBank::read (tab, tilt, phase) * g.killGain * fade.next();
                L[n] = l;
                R[n] = r;
                ++n;

                g.readAbs += g.step;
                ++g.age;
                if (g.killStep > 0.0f)
                    g.killGain = std::max (0.0f, g.killGain - g.killStep);

                if (! ((static_cast<double> (g.age) + g.frac) < g.len && g.killGain > 0.0f))
                {
                    retire = true;
                    break;
                }
            }
        }

        // FX, in ARCHITECTURE §9 order. The flags are latched per grain.
        const std::uint32_t fxa = g.fxActive;
        if (fxa != 0)
        {
            if ((fxa & kFxSvf) != 0)
            {
                fx::svf (g, L, R, n);
                // P25 NaN kill: a non-finite integrator retires the grain and its
                // segment contributes nothing (FTZ does not catch NaN).
                bool finite = fx::svfStateFinite (g);
               #if OGRAN_TEST_HOOKS
                finite = finite && ! forcedRetire;
               #endif
                if (! finite)
                {
                   #if OGRAN_TEST_HOOKS
                    ++diag.nanKills;
                    recordRetire (g, slot);
                    if (numNanKills < kPoolLive + kPoolFade)
                        nanKillSlots[numNanKills++] = slot;
                   #endif
                    return false;
                }
            }
            if ((fxa & kFxDrive) != 0)
                fx::drive (g, L, R, n);
            if ((fxa & kFxCrush) != 0)
                fx::crush (g, L, R, n);
            if ((fxa & kFxRing) != 0)
                fx::ring (g, L, R, n);
        }

        float* const bl = bus[g.stream][0] + from;
        float* const br = bus[g.stream][1] + from;
        for (int j = 0; j < n; ++j)
        {
            bl[j] += W[j] * L[j] * g.gainL;
            br[j] += W[j] * R[j] * g.gainR;
        }

       #if OGRAN_TEST_HOOKS
        if (retire)
            recordRetire (g, slot);
       #endif
        return ! retire;
    });

    if (applyKills)
        spawner.pendingKillsApplied();

   #if OGRAN_TEST_HOOKS
    for (int k = 0; k < numNanKills; ++k)
        if (pool.isActive (nanKillSlots[k]))
            ++diag.nanKillNotRetired;
    numNanKills = 0;
   #endif
}

void Engine::mixOut (const float* dryL, const float* dryR, float* outL, float* outR,
                     int numOut, int len, int cellPos) noexcept
{
    // 6. Stream mix → wet[2][kCell], ONCE per piece (P41) — the 2.3
    //    `wl += bus·gain` statement verbatim (FP contraction is per statement).
    for (int i = 0; i < len; ++i)
    {
        const int c = cellPos + i;
        float wl = 0.0f, wr = 0.0f, tl = 0.0f, tr = 0.0f;
        for (int s = 0; s < kMaxStreams; ++s)
        {
            wl += bus[s][0][i] * streamGainL[s].at (c);
            wr += bus[s][1][i] * streamGainR[s].at (c);
            tl += bus[s][0][i] * fbTapCell[s].at (c);   // feedback tap: pre-g_out, no pan (R31)
            tr += bus[s][1][i] * fbTapCell[s].at (c);
        }
        wet[0][i] = wl;
        wet[1][i] = wr;
        tap[0][i] = tl;
        tap[1][i] = tr;
    }

    // 8. reverb → D9 knee, BEFORE the mix-0 fast path: neither one's state may
    //    depend on the outer mix (R35); the fast path still outputs dry bitwise.
    {
        float* const chans[2] { wet[0], wet[1] };
        revView.setDataToReferTo (chans, 2, len);
       #if OGRAN_TEST_HOOKS
        float before[2][kCell];
        std::copy (wet[0], wet[0] + len, before[0]);
        std::copy (wet[1], wet[1] + len, before[1]);
       #endif
        reverb.process (revView);
       #if OGRAN_TEST_HOOKS
        if (std::memcmp (before[0], wet[0], sizeof (float) * (size_t) len) != 0
            || std::memcmp (before[1], wet[1], sizeof (float) * (size_t) len) != 0)
            ++diag.reverbCells;
       #endif
    }
    bool kneeOn = true;
   #if OGRAN_TEST_HOOKS
    kneeOn = ! hooks.legacyKnee;
   #endif
    if (kneeOn)
        for (int i = 0; i < len; ++i)
        {
            wet[0][i] = wetKnee (wet[0][i]);
            wet[1][i] = wetKnee (wet[1][i]);
        }
   #if OGRAN_TEST_HOOKS
    for (int i = 0; i < len; ++i)
        diag.maxWetPeak = std::max ({ diag.maxWetPeak, std::abs (wet[0][i]), std::abs (wet[1][i]) });
   #endif

    // P4 / R2: with mix settled at exactly 0 and output gain settled at exactly
    // 1.0f for the whole cell, out = dry by COPY. dry·1 + wet·0 is not bitwise
    // (signed zeros, NaN/Inf wet). The grains above still ran, so the engine
    // stays warm and the draw sequence is unchanged.
    const ExactRamp& outGain = useOutMod ? outGainMod : outGainRamp;   // 2.5a: routed output_gain (R47)
    if (wetGain.settledAt (0.0f) && dryGain.settledAt (1.0f) && outGain.settledAt (1.0f))
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
        const float wl = wet[0][i];
        const float wr = wet[1][i];

        const float dg = dryGain.at (c);
        const float wg = wetGain.at (c);
        const float og = outGain.value (c);
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
