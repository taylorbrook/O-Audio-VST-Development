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
#include "Engine.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace ogi
{
namespace
{
    bool sameBits (const void* a, const void* b, std::size_t n) noexcept
    {
        return std::memcmp (a, b, n) == 0;
    }
}

//==============================================================================
Engine::Engine (SlotHandoff& h)
    : handoff (h)
{
   #if OGI_TEST_HOOKS
    hooks.log.assign (static_cast<std::size_t> (EngineTestHooks::kLogCapacity), GrainLogEntry {});
    for (int i = 0; i < kVoiceSlots; ++i)
        hooks.slotPerm[i] = i;
   #endif
    voiceGainLin = juce::Decibels::decibelsToGain (static_cast<float> (kVoiceGainDb));
    voiceGainNow = voiceGainLin;
    prepare (44100.0, 512);
}

Engine::~Engine()
{
    // The loader (the only other producer) is already gone and process() is not
    // running: every tape the engine holds is freed here, on the destroying thread.
    for (auto*& t : current)
    {
        delete t;
        t = nullptr;
    }
    for (int i = 0; i < numRetiring; ++i)
    {
        delete retiring[i];
        retiring[i] = nullptr;
    }
    numRetiring = 0;
}

void Engine::bind (juce::AudioProcessorValueTreeState& apvts)
{
    ptrs.bind (apvts);
    gp.latch (ptrs);
}

void Engine::bindScale (const ScaleSource& source) noexcept
{
    scale.bind (source);
}

void Engine::prepare (double sampleRate, int maxBlockSize)
{
    juce::ignoreUnused (maxBlockSize);   // every scratch array is one 32-sample cell
    fs = sampleRate > 0.0 ? sampleRate : 44100.0;
    absPos = 0;
    serialCounter = 0;
    bend = 0.0f;

    pool.prepare (fs, kPoolFadeMs);
    spawner.prepare (fs, kKillFadeMs);
    for (auto& v : voices)
        v = Voice {};

    // The pool is empty now: no crossfade can be running and nothing holds the
    // retiring tapes. (prepareToPlay never runs concurrently with process(), so
    // this thread is the ring's only producer while it runs.)
    for (auto& x : xfade)
        x = XfadeState {};
    int keep = 0;
    for (int i = 0; i < numRetiring; ++i)
        if (! handoff.retire (retiring[i]))
            retiring[keep++] = retiring[i];
    for (int i = keep; i < numRetiring; ++i)
        retiring[i] = nullptr;
    numRetiring = keep;

    fbReturn.assign (static_cast<std::size_t> (2 * kFbDelay), 0.0f);

    rampCells20 = rampCellsFor (fs);
    rampCells10 = rampCellsFor (fs, kMixerRampMs);

    gp.latch (ptrs);
    outGainRamp.reset (dbToGain (gp.outputGainDb));
    setStreamRamps (true);
    for (int s = 0; s < kGiStreams; ++s)
    {
        prevEnable[s] = gp.s[s].enable;
        jit[s] = jitterOf (s);
    }

    updateRegion();
    for (int s = 0; s < kGiStreams; ++s)
        freeRun[s].restart (0, static_cast<double> (gp.s[s].scanRate) / fs);

    // PD3: the only blocking copy of the custom table (process() not running).
    scale.copyCustomTable (true);
    scale.invalidate();
    scale.update (gp);

    quotas = QuotasN<kPairs> {};
    quotasDirty = true;
    prevCap = -1;
    for (int s = 0; s < kGiStreams; ++s)
        prevLMax[s] = prevDensity[s] = prevJitter[s] = -1.0;
    std::fill (std::begin (prevDemand), std::end (prevDemand), 0.0);
    std::fill (std::begin (prevLawKey), std::end (prevLawKey), 0.0);
    for (int k = 0; k < kVoiceSlots; ++k)
        for (int s = 0; s < kGiStreams; ++s)
        {
            quotaM[k][s] = kQuotaUncapped;
            periodT[k][s] = uncappedPeriod (s);
            lawTarget[k][s] = 1.0f;
            lawGain[k][s].reset (1.0f);
            coher[k][s] = 0.0;
        }

    for (auto& row : envBuf)
        std::fill (std::begin (row), std::end (row), 0.0f);
    bus = VoiceBus {};

   #if OGI_TEST_HOOKS
    for (int k = 0; k < kVoiceSlots; ++k)
        writeVoiceHooks (k);
    hooks.liveNow = hooks.activeNow = 0;
    hooks.retiringNow = numRetiring;
   #endif
}

//==============================================================================
// Helpers
AmpParams Engine::ampParams() const noexcept
{
    AmpParams p;
    p.attackMs  = gp.ampAttackMs;
    p.decayMs   = gp.ampDecayMs;
    p.sustain   = gp.ampSustain;
    p.releaseMs = gp.ampReleaseMs;
    return p;
}

double Engine::voiceSemis (const Voice& v, int s) const noexcept
{
    // ARCHITECTURE Algorithm §2: st = κ·(note + bend − root − fine/100) + pitch
    // (before the random term and the ±48 clamp).
    const auto& sp = gp.s[s];
    const double key = static_cast<double> (v.note) - static_cast<double> (sp.rootNote)
                     - static_cast<double> (sp.rootFineCt) / 100.0
                     + static_cast<double> (bend) * static_cast<double> (gp.pitchBendRangeSt);
    const double st0 = static_cast<double> (sp.keyTrack) * key + static_cast<double> (sp.pitchSt);
    return st0;
}

double Engine::voiceRate (const Voice& v, const SlotTape& tape, int s) const noexcept
{
    // st clamped to ±48; rate = 2^(st/12) · tapeRate / fs (double).
    const double st0 = voiceSemis (v, s);
    const double st  = std::clamp (st0, -kMaxSemis, kMaxSemis);
    return std::exp2 (st / 12.0) * tape.rate / fs;
}

double Engine::rateForNote (int note, int s) const noexcept
{
    s = std::clamp (s, 0, kGiStreams - 1);
    const SlotTape* tp = current[(size_t) s];
    if (tp == nullptr)
        return 0.0;
    Voice v;
    v.note = note;
    return voiceRate (v, *tp, s);
}

double Engine::uncappedPeriod (int s) const noexcept
{
    return thinnedPeriod (fs, static_cast<double> (gp.s[s].density), lMaxSamples (s), kQuotaUncapped);
}

double Engine::lMaxSamples (int s) const noexcept
{
    const auto& sp = gp.s[s];
    const double ms = static_cast<double> (sp.sizeMs) * (1.0 + 0.5 * static_cast<double> (sp.sizeRand));
    return std::clamp (ms, kMinSizeMs, kMaxSizeMs) * 0.001 * fs;
}

double Engine::lenBarSamples (int s) const noexcept
{
    return std::clamp (static_cast<double> (gp.s[s].sizeMs), kMinSizeMs, kMaxSizeMs) * 0.001 * fs;
}

double Engine::jitterOf (int s) const noexcept
{
    double J = static_cast<double> (gp.s[s].jitter);
   #if OGI_TEST_HOOKS
    J *= hooks.jitterScale;
   #endif
    return std::clamp (J, 0.0, 1.0);
}

bool Engine::audibleTarget (int s, bool anySolo) const noexcept
{
    // PD9 / R§6: enable folded in (10 ms enable ramp); additive solo over ENABLED streams.
    const auto& sp = gp.s[s];
    return sp.enable && ! sp.mute && (! anySolo || sp.solo);
}

/** Telescoping jitter draw u_k (PD13): u_0 := ½ (no draw), else keyed on
    (serial << 24) | ((epoch + k) & 0xFFFFFF) on kSchedStream + s. */
double Engine::jitterU (int s, std::uint64_t serial, std::uint64_t epoch, std::uint64_t k) const noexcept
{
    if (k == 0)
        return 0.5;
    const std::uint64_t idx = epoch + k;
    const ouaricon::grain::CounterRng::Draw d (seed, kSchedStream + static_cast<std::uint64_t> (s),
                                                (serial << 24) | (idx & 0xFFFFFFull));
    return static_cast<double> (d.nextFloat (Slot::Jitter));
}

/** Gap after onset k: T·(1 + J·(u_{k+1} − u_k)); J = 0 → T exactly, no draw. */
double Engine::jitterGap (double T, double J, int s, std::uint64_t serial, std::uint64_t epoch, std::uint64_t k) const noexcept
{
    if (! (J > 0.0))
        return T;
    return T * (1.0 + J * (jitterU (s, serial, epoch, k + 1) - jitterU (s, serial, epoch, k)));
}

void Engine::setStreamRamps (bool snap) noexcept
{
    bool anySolo = false;
    for (int s = 0; s < kGiStreams; ++s)
        anySolo = anySolo || (gp.s[s].enable && gp.s[s].solo);

    bool zero = false;
   #if OGI_TEST_HOOKS
    zero = hooks.mixerRampZero;
   #endif
    for (int s = 0; s < kGiStreams; ++s)
    {
        const auto& sp = gp.s[s];
        const float lvl = levelDbToGain (sp.levelDb);
        const auto  pg  = panGains (sp.pan);
        const float aud = audibleTarget (s, anySolo) ? 1.0f : 0.0f;
        if (snap)
        {
            levelRamp[s].reset (lvl);
            panLRamp[s].reset (pg.a);
            panRRamp[s].reset (pg.b);
            audibleRamp[s].reset (aud);
        }
        else if (zero)
        {
            // MX1 control: a 0 ms step at the cell boundary.
            levelRamp[s].setCell (lvl, lvl);
            panLRamp[s].setCell (pg.a, pg.a);
            panRRamp[s].setCell (pg.b, pg.b);
            audibleRamp[s].setCell (aud, aud);
        }
        else
        {
            levelRamp[s].setTarget (lvl, rampCells20);
            levelRamp[s].tick();
            panLRamp[s].setTarget (pg.a, rampCells20);
            panLRamp[s].tick();
            panRRamp[s].setTarget (pg.b, rampCells20);
            panRRamp[s].tick();
            audibleRamp[s].setTarget (aud, rampCells10);
            audibleRamp[s].tick();
        }
    }
}

double Engine::pairCoherence (int k, int s) const noexcept
{
   #if OGI_TEST_HOOKS
    if (hooks.coherenceOff)
        return 0.0;
   #endif
    const auto& sp = gp.s[s];
    const SlotTape* tp = current[(size_t) s];
    const Voice& v = voices[k];
    if (tp == nullptr || ! (tp->rate > 0.0) || v.isFree())
        return 0.0;

    // Span in OUTPUT samples (the parent's spanSamples): W tape frames / tapeRate · fs.
    const double span = region[s].width() * fs / tp->rate;
    const PitchSel& sel = scale.selection (s);
    const bool scaleOn = sel.set != nullptr;

    // PD2 option (b): the "pitch" argument is the relative velocity
    // 12·|log2 (r_kS / v_s)| — r_kS the voice's forward rate on stream s,
    // v_s = scanRate_s · tapeRate_s / fs; v_s <= 0 → +∞ → c_pitch = 0.
    double pitchArg = std::numeric_limits<double>::infinity();
    bool verbatim = false;
   #if OGI_TEST_HOOKS
    verbatim = hooks.coherenceVerbatim;
   #endif
    if (verbatim)
    {
        pitchArg = static_cast<double> (sp.pitchSt);   // option (a), CO1 control
    }
    else
    {
        const double r     = voiceRate (v, *tp, s);
        const double vScan = static_cast<double> (sp.scanRate) * tp->rate / fs;
        if (vScan > 0.0 && r > 0.0)
            pitchArg = 12.0 * std::abs (std::log2 (r / vScan));
    }
    return Coherence::compute (static_cast<double> (sp.reverseProb), static_cast<double> (sp.spray), span, fs,
                               pitchArg, static_cast<double> (sp.pitchRand), scaleOn,
                               scaleOn ? sel.hi - sel.lo : 0, scaleOn && sel.set->hasUnison());
}

/** PD2 law for pair (k, s): g = outputGainFromPower (streamPower (n, n, win_s,
    c, spread_s)). With c = 0, spread 0 and window 0 / skew 0 this is the 2.1
    GiRenderer::voiceLawGain expression, bitwise. */
float Engine::pairLaw (int k, int s, double nEff) noexcept
{
    const auto& sp = gp.s[s];
    const double c = pairCoherence (k, s);
    coher[k][s] = c;
   #if OGI_TEST_HOOKS
    if (hooks.lawOff)
        return 1.0f;
   #endif
    WindowSel win;
    win.step = WindowMorphBank::stepFor (sp.window);
    win.tilt = ouaricon::grain::WindowLut::makeTilt (0.5f + 0.45f * sp.windowSkew);
    const double p = streamPower (nEff, nEff, win, c, static_cast<double> (sp.spread), morphBank);
    return outputGainFromPower (p);
}

void Engine::fillLawKey (double (&key)[kLawKeySize]) const noexcept
{
    int i = 0;
    for (int s = 0; s < kGiStreams; ++s)
    {
        const auto& sp = gp.s[s];
        const SlotTape* tp = current[(size_t) s];
        const PitchSel& sel = scale.selection (s);
        key[i++] = sp.enable ? 1.0 : 0.0;
        key[i++] = static_cast<double> (sp.window);
        key[i++] = static_cast<double> (sp.windowSkew);
        key[i++] = static_cast<double> (sp.spread);
        key[i++] = static_cast<double> (sp.reverseProb);
        key[i++] = static_cast<double> (sp.spray);
        key[i++] = static_cast<double> (sp.pitchRand);
        key[i++] = static_cast<double> (sp.pitchSt);
        key[i++] = static_cast<double> (sp.keyTrack);
        key[i++] = static_cast<double> (sp.rootNote);
        key[i++] = static_cast<double> (sp.rootFineCt);
        key[i++] = static_cast<double> (sp.scanRate);
        key[i++] = region[s].width();
        key[i++] = tp != nullptr ? tp->rate : 0.0;
        key[i++] = sel.set != nullptr ? static_cast<double> (sel.hi - sel.lo) : -1.0;
        key[i++] = (sel.set != nullptr && sel.set->hasUnison()) ? 1.0 : 0.0;
    }
    key[i++] = static_cast<double> (bend);
    key[i++] = static_cast<double> (gp.pitchBendRangeSt);
    double hk[3] { 0.0, 0.0, 0.0 };
   #if OGI_TEST_HOOKS
    hk[0] = hooks.coherenceOff ? 1.0 : 0.0;
    hk[1] = hooks.coherenceVerbatim ? 1.0 : 0.0;
    hk[2] = hooks.lawOff ? 1.0 : 0.0;
   #endif
    key[i++] = hk[0];
    key[i++] = hk[1];
    key[i++] = hk[2];
    jassert (i == kLawKeySize);
}

void Engine::writeVoiceHooks (int k) noexcept
{
   #if OGI_TEST_HOOKS
    const Voice& v = voices[k];
    hooks.voiceSerial[k] = v.serial;
    hooks.voiceNote[k]   = v.note;
    hooks.voiceState[k]  = v.isCounted() ? 1 : (v.isStolen() ? 2 : 0);
    if (! v.isFree() && current[0] != nullptr)
    {
        hooks.voiceRate[k]  = voiceRate (v, *current[0], 0);
        hooks.voiceLevel[k] = ouaricon::grain::latchLevel (hooks.voiceRate[k], kTapeLevels);
    }
    else if (v.isFree())
    {
        hooks.voiceRate[k]  = 0.0;
        hooks.voiceLevel[k] = 0;
    }
   #else
    juce::ignoreUnused (k);
   #endif
}

//==============================================================================
void Engine::process (float* outL, float* outR, int numSamples, const juce::MidiBuffer& midi,
                      std::uint64_t seedIn) noexcept
{
    if (numSamples <= 0 || outL == nullptr || outR == nullptr)
        return;

    seed = seedIn;
    voiceGainNow = voiceGainLin;
   #if OGI_TEST_HOOKS
    if (hooks.seedOverrideOn)
        seed = hooks.seedOverride;
    if (! std::isnan (hooks.voiceGainDbOverride))
        voiceGainNow = juce::Decibels::decibelsToGain (static_cast<float> (hooks.voiceGainDbOverride));
   #endif

    MidiCursor cursor (midi, numSamples);
    const std::int64_t blockStart = absPos;
    int pos = 0;
    while (pos < numSamples)
    {
        const std::int64_t t = blockStart + pos;
        const int cellOff = static_cast<int> (t & static_cast<std::int64_t> (kCell - 1));
        if (cellOff == 0)
            gridTick (t);

        const int len = std::min (numSamples - pos, kCell - cellOff);
        const int b = pos + len;

        int seg = pos;
        while (cursor.nextPos() < b)
        {
            const MidiEvent e = cursor.pop();
            const int at = std::max (e.pos, seg);
            if (at > seg)
            {
                runSegment (cellOff + (seg - pos), cellOff + (at - pos), blockStart + seg);
                seg = at;
            }
            applyEvent (e, blockStart + at);
        }
        if (b > seg)
            runSegment (cellOff + (seg - pos), cellOff + (b - pos), blockStart + seg);

        mixPiece (cellOff, cellOff + len, outL + pos, outR + pos);
        pos = b;
    }
    absPos = blockStart + numSamples;
}

//==============================================================================
// Grid tick
void Engine::gridTick (std::int64_t t) noexcept
{
    gp.latch (ptrs);

    for (int s = 0; s < kGiStreams; ++s)
    {
        jit[s] = jitterOf (s);

        // P14: an enable edge while a voice sounds folds the onset count into
        // the epoch and restarts the scheduler at this tick (draws never replay).
        const bool en = gp.s[s].enable;
        if (en && ! prevEnable[s])
        {
            for (auto& v : voices)
            {
                if (! v.isCounted())
                    continue;
                v.epoch[s] += v.sched[s].getOnsetCount();
                v.sched[s].reset();
            }
        }
        prevEnable[s] = en;
    }

    outGainRamp.setTarget (dbToGain (gp.outputGainDb), rampCells20);
    outGainRamp.tick();
    setStreamRamps (false);

    takeTapes (t);
    finishXfades (t);
    retireScan();
    updateRegion();
    enforcePolyphony();
    updateScale();
    updateQuotas();
    updateScan (t);

   #if OGI_TEST_HOOKS
    if (hooks.poisonFreeSlots || hooks.fxSubnormalScan)
    {
        // The pool's array is non-const storage; the hooks write it only here,
        // between renders, and only for INACTIVE slots (PD12).
        auto* gs = const_cast<VoiceGrain*> (pool.getGrains().data());
        for (int j = 0; j < GiPool::kNumSlots; ++j)
        {
            if (pool.isActive (j))
            {
                if (hooks.fxSubnormalScan)
                    hooks.fxSubnormals += (std::uint64_t) fxstate::subnormals (gs[j].g);
            }
            else if (hooks.poisonFreeSlots)
            {
                fxstate::poison (gs[j].g);
                fxstate::copy (hooks.fxShadow[j], gs[j].g);
            }
        }
    }
    ++hooks.gridTicks;
    hooks.liveNow   = spawner.countAlive (pool);
    hooks.liveMin   = std::min (hooks.liveMin, hooks.liveNow);
    hooks.liveMax   = std::max (hooks.liveMax, hooks.liveNow);
    hooks.activeNow = pool.countActive();
    spawner.countAlivePerVoice (pool, hooks.liveVoice);
    spawner.countAlivePerPair (pool, hooks.liveVS);
    for (int k = 0; k < kVoiceSlots; ++k)
    {
        for (int s = 0; s < kGiStreams; ++s)
        {
            hooks.quotaMVS[k][s]    = quotaM[k][s];
            hooks.quotaQVS[k][s]    = quotas.q[k * kGiStreams + s];
            hooks.periodTVS[k][s]   = periodT[k][s];
            hooks.lawTargetVS[k][s] = lawTarget[k][s];
            hooks.coherenceVS[k][s] = coher[k][s];
        }
        hooks.quotaM[k]  = quotaM[k][0];
        hooks.quotaQ[k]  = quotas.q[k * kGiStreams];
        hooks.periodT[k] = periodT[k][0];
        writeVoiceHooks (k);
    }
    hooks.lambda   = quotas.lambda;
    hooks.capped   = quotas.capped;
    hooks.regionLo = region[0].lo;
    hooks.regionHi = region[0].hi;
    for (int s = 0; s < kGiStreams; ++s)
    {
        hooks.regionLoS[s] = region[s].lo;
        hooks.regionHiS[s] = region[s].hi;
    }
    for (int s = 0; s < kTapeSlots; ++s)
    {
        hooks.xfadeActive[s] = xfade[s].active;
        hooks.tapeRate[s]    = current[(size_t) s] != nullptr ? current[(size_t) s]->rate : 0.0;
    }
    hooks.retiringNow = numRetiring;
   #endif
}

void Engine::takeTapes (std::int64_t t) noexcept
{
    for (int s = 0; s < kTapeSlots; ++s)
    {
        if (xfade[s].active)
            continue;                      // one crossfade per slot at a time; the tape waits
        if (numRetiring >= kMaxRetiring)
            break;                         // back-pressure: pending tapes wait in their slot

        SlotTape* p = handoff.take (s);
        if (p == nullptr)
            continue;

       #if OGI_TEST_HOOKS
        ++hooks.tapesTaken;
       #endif
        SlotTape* old = current[(size_t) s];
        current[(size_t) s] = p;
        if (old == nullptr)
            continue;

        if (p->rebuild && old->generation == p->generation && old->rate > 0.0 && p->rate > 0.0)
        {
            // Same source re-rated (SR change): 20 ms dual-read crossfade. Grains
            // on the old tape are rebased lazily at their next render.
            auto& x = xfade[s];
            x.active  = true;
            x.oldTape = old;
            x.newTape = p;
            x.ratio   = p->rate / old->rate;
            x.start   = t;
            x.len     = std::max (1, static_cast<int> (std::lround (kXfadeMs * 0.001 * fs)));
           #if OGI_TEST_HOOKS
            ++hooks.xfadesStarted;
           #endif
        }
        else
        {
            // New file / recording: the old tape's grains get the 5 ms kill fade;
            // new spawns read the new tape at once.
            killGrainsOfTape (old);
        }
        retiring[numRetiring++] = old;
    }
}

void Engine::killGrainsOfTape (const SlotTape* tape) noexcept
{
    const VoiceGrain* const base = pool.getGrains().data();
    for (int i = 0; i < kPoolLive; ++i)   // fade slots already fade out within 5 ms
    {
        if (! pool.isActive (i) || spawner.isKilling (base[i], i))
            continue;
        if (base[i].tape == tape || base[i].xTape == tape)
        {
            spawner.requestKill (i);
           #if OGI_TEST_HOOKS
            ++hooks.swapKills;
           #endif
        }
    }
}

void Engine::finishXfades (std::int64_t t) noexcept
{
    for (auto& x : xfade)
        if (x.active && t >= x.start + static_cast<std::int64_t> (x.len))
            x = XfadeState {};   // grains drop their xTape at their next render
}

void Engine::retireScan() noexcept
{
    if (numRetiring == 0)
        return;

    const VoiceGrain* const base = pool.getGrains().data();
    int scanSlots = GiPool::kNumSlots;   // P12: live AND fade slots (288)
   #if OGI_TEST_HOOKS
    if (hooks.retireScan256)
        scanSlots = kPoolLive;
   #endif

    int keep = 0;
    for (int i = 0; i < numRetiring; ++i)
    {
        SlotTape* tp = retiring[i];
        bool held = false;
        for (const auto& x : xfade)
            if (x.active && (x.oldTape == tp || x.newTape == tp))
                held = true;

        bool heldAnywhere = held;
        for (int j = 0; j < GiPool::kNumSlots && ! heldAnywhere; ++j)
        {
            if (! pool.isActive (j))
                continue;
            if (base[j].tape == tp || base[j].xTape == tp)
            {
                heldAnywhere = true;
                if (j < scanSlots)
                    held = true;
            }
        }

        if (! held && heldAnywhere)
        {
            // Only reachable with the H3 control hook: the reduced scan would have
            // retired a tape a fade spare still reads. Count it; keep the tape.
           #if OGI_TEST_HOOKS
            ++hooks.earlyRetires;
           #endif
            retiring[keep++] = tp;
            continue;
        }

        if (! held && handoff.retire (tp))
        {
           #if OGI_TEST_HOOKS
            ++hooks.retiredByEngine;
           #endif
            continue;
        }
        retiring[keep++] = tp;
    }
    for (int i = keep; i < numRetiring; ++i)
        retiring[i] = nullptr;
    numRetiring = keep;
}

void Engine::updateRegion() noexcept
{
    // Stream s reads slot s (CONTEXT-2.2 Defaults).
    for (int s = 0; s < kGiStreams; ++s)
    {
        const SlotTape* tp = current[(size_t) s];
        if (tp == nullptr)
        {
            region[s] = LoopRegion {};
            continue;
        }
        const auto& sp = gp.s[s];
        region[s] = loopRegion (static_cast<double> (sp.loopStart), static_cast<double> (sp.loopEnd),
                                tp->frames, kMinLoopMs * 0.001 * tp->rate);
    }
}

void Engine::enforcePolyphony() noexcept
{
    const int poly = std::clamp (gp.polyphony, 1, kMaxCountedVoices);
    int counted = VoiceAllocator::countCounted (voices);
    while (counted > poly)
    {
        const int k = VoiceAllocator::oldestCounted (voices);
        if (k < 0)
            break;
        forceReleaseVoice (k);
        --counted;
       #if OGI_TEST_HOOKS
        ++hooks.polyForceReleases;
       #endif
    }
}

void Engine::updateScale() noexcept
{
    // PD3: non-blocking; a missed try-lock retries at the next tick.
    const auto r = scale.copyCustomTable (false);
    const int rebuilds = scale.update (gp);
   #if OGI_TEST_HOOKS
    if (r == GiScaleState::CopyResult::copied) ++hooks.customCopies;
    if (r == GiScaleState::CopyResult::missed) ++hooks.customTryFails;
    hooks.activeSetRebuilds += static_cast<std::uint64_t> (rebuilds);
   #else
    juce::ignoreUnused (r, rebuilds);
   #endif
}

void Engine::updateQuotas() noexcept
{
    // Demand index k·4 + s: N = streamDemand for a counted voice × enabled
    // stream × loaded slot, else 0. Muted streams KEEP their demand (PD9).
    double n[kGiStreams];
    bool canSpawn[kGiStreams];
    double lMax[kGiStreams], density[kGiStreams];
    for (int s = 0; s < kGiStreams; ++s)
    {
        const auto& sp = gp.s[s];
        canSpawn[s] = sp.enable && current[(size_t) s] != nullptr;
        n[s]        = streamDemand (static_cast<double> (sp.density), static_cast<double> (sp.sizeMs), fs);
        lMax[s]     = lMaxSamples (s);
        density[s]  = static_cast<double> (sp.density);
    }

    double demand[kPairs];
    for (int k = 0; k < kVoiceSlots; ++k)
        for (int s = 0; s < kGiStreams; ++s)
            demand[k * kGiStreams + s] = (canSpawn[s] && voices[k].isCounted()) ? n[s] : 0.0;

    bool thinOff = false, periodOff = false;
   #if OGI_TEST_HOOKS
    thinOff   = hooks.thinningOff;
    periodOff = hooks.periodThinOff;
   #endif
    const int thinMode = (thinOff ? 1 : 0) + (periodOff ? 2 : 0);

    double lawKey[kLawKeySize];
    fillLawKey (lawKey);

    const bool changed = quotasDirty || prevCap != gp.grainLimit || thinMode != prevThinOff
                      || ! sameBits (lMax, prevLMax, sizeof (lMax)) || ! sameBits (density, prevDensity, sizeof (density))
                      || ! sameBits (jit, prevJitter, sizeof (jit))
                      || ! sameBits (demand, prevDemand, sizeof (demand))
                      || ! sameBits (lawKey, prevLawKey, sizeof (lawKey));
    if (! changed)
    {
        // Inputs unchanged: hold every law gain (a CellLerp needs a push per tick).
        for (int k = 0; k < kVoiceSlots; ++k)
            for (int s = 0; s < kGiStreams; ++s)
                lawGain[k][s].push (lawTarget[k][s]);
        return;
    }

    quotas = waterFillN (demand, gp.grainLimit);
    std::memcpy (prevDemand, demand, sizeof (demand));
    std::memcpy (prevLMax, lMax, sizeof (lMax));
    std::memcpy (prevDensity, density, sizeof (density));
    std::memcpy (prevJitter, jit, sizeof (jit));
    std::memcpy (prevLawKey, lawKey, sizeof (lawKey));
    prevCap = gp.grainLimit;
    prevThinOff = thinMode;
    quotasDirty = false;

    double lenBar[kGiStreams];
    for (int s = 0; s < kGiStreams; ++s)
        lenBar[s] = lenBarSamples (s);

    for (int k = 0; k < kVoiceSlots; ++k)
    {
        Voice& v = voices[k];
        for (int s = 0; s < kGiStreams; ++s)
        {
            // A disabled (k, s) holds the uncapped form (R§6 / PLAN T8).
            int m = thinOff ? kQuotaUncapped : quotas.m[k * kGiStreams + s];
            if (! gp.s[s].enable)
                m = kQuotaUncapped;
            quotaM[k][s] = m;
            const double J = jit[s];
            const double T = thinnedPeriod (fs, density[s], lMax[s], periodOff ? kQuotaUncapped : m, J);
            // R§1.3 / R§8: a quota DECREASE re-times the pending gap at once with
            // the jittered gap of the pending onset (a pure function of its index
            // and the grid snapshot: block-size invariant); an increase lets the
            // latched gap roll.
            if (v.isCounted() && T > periodT[k][s])
            {
                const std::uint64_t cnt = v.sched[s].getOnsetCount();
                const double gap = cnt > 0 ? jitterGap (T, J, s, v.serial, v.epoch[s], cnt - 1) : T;
                v.sched[s].rescheduleNext (gap);
            }
            periodT[k][s] = T;
            lawTarget[k][s] = pairLaw (k, s, effectiveCount (lenBar[s], T));
            lawGain[k][s].push (lawTarget[k][s]);
        }
    }
}

void Engine::updateScan (std::int64_t t) noexcept
{
    for (int s = 0; s < kGiStreams; ++s)
    {
        const auto& sp = gp.s[s];
        const double rateSec  = static_cast<double> (sp.scanRate) / fs;   // tape seconds per output sample
        const double tapeRate = current[(size_t) s] != nullptr ? current[(size_t) s]->rate : 0.0;
        freeRun[s].reanchor (t, rateSec, scanPeriodSec (region[s], tapeRate, kScanFreeRun));
        const double voicePeriod = scanPeriodSec (region[s], tapeRate, sp.scanMode);
        for (auto& v : voices)
            if (! v.isFree())
                v.scan[s].reanchor (t, rateSec, voicePeriod);
    }
}

//==============================================================================
// Events
void Engine::applyEvent (const MidiEvent& e, std::int64_t t) noexcept
{
    if (e.numBytes < 1)
        return;
    const int type    = e.status & 0xF0;
    const int channel = (e.status & 0x0F) + 1;

    if (type == 0x90 && e.numBytes >= 3)
    {
        if (e.data2 > 0)
            noteOn (channel, e.data1 & 0x7F, static_cast<float> (e.data2) / 127.0f, t);
        else
            noteOff (channel, e.data1 & 0x7F);
    }
    else if (type == 0x80 && e.numBytes >= 3)
    {
        noteOff (channel, e.data1 & 0x7F);
    }
    else if (type == 0xE0 && e.numBytes >= 3)
    {
        const int value = (e.data1 & 0x7F) | ((e.data2 & 0x7F) << 7);
        const int d = value - 8192;
        bend = d >= 0 ? static_cast<float> (d) / 8191.0f : static_cast<float> (d) / 8192.0f;
    }
    else if (type == 0xB0 && e.numBytes >= 3)
    {
        if (e.data1 == 120)
            allSoundOff();
        else if (e.data1 == 123)
            allNotesOff();
    }
}

void Engine::noteOn (int channel, int note, float velocity01, std::int64_t t) noexcept
{
    const int poly = std::clamp (gp.polyphony, 1, kMaxCountedVoices);
    if (VoiceAllocator::countCounted (voices) >= poly)
    {
        const int victim = VoiceAllocator::chooseVictim (voices, note);
        if (victim >= 0)
            stealVoice (victim);
    }

    // When every spare is still fading, the quietest one is ended.
    while (VoiceAllocator::countStolen (voices) > kStealSpares)
        if (! endQuietestStolen())
            break;

    const int* order = nullptr;
   #if OGI_TEST_HOOKS
    if (hooks.slotPermOn)
        order = hooks.slotPerm;
   #endif
    int k = VoiceAllocator::findFree (voices, order);
    if (k < 0 && order != nullptr)
        k = VoiceAllocator::findFree (voices, nullptr);
    while (k < 0 && endQuietestStolen())
        k = VoiceAllocator::findFree (voices, nullptr);
    if (k < 0)
        return;

    startVoice (k, channel, note, velocity01, t);
}

void Engine::startVoice (int k, int channel, int note, float velocity01, std::int64_t t) noexcept
{
    Voice& v = voices[k];
    v = Voice {};
    v.state     = Voice::State::active;
    v.note      = note;
    v.channel   = channel;
    v.velocity  = velocity01;
    v.gated     = true;
    v.serial    = ++serialCounter;
    v.noteOnAbs = t;
    v.env.noteOn (ampParams(), fs, AmpEnv::velocityGain (velocity01, gp.velToAmp));
    for (int s = 0; s < kGiStreams; ++s)
    {
        v.sched[s].reset();                    // first onset exactly at the note-on sample
        v.scan[s].restart (t, static_cast<double> (gp.s[s].scanRate) / fs);
    }

    // Until the next grid tick includes it, the new voice runs unthinned.
    for (int s = 0; s < kGiStreams; ++s)
    {
        quotaM[k][s]  = kQuotaUncapped;
        periodT[k][s] = uncappedPeriod (s);
        const double n = streamDemand (static_cast<double> (gp.s[s].density), static_cast<double> (gp.s[s].sizeMs), fs);
        lawTarget[k][s] = pairLaw (k, s, n);
        lawGain[k][s].reset (lawTarget[k][s]);
    }
    quotasDirty = true;

    writeVoiceHooks (k);
}

void Engine::noteOff (int channel, int note) noexcept
{
    const AmpParams p = ampParams();
    for (auto& v : voices)
    {
        if (v.isKeyDown() && v.note == note && v.channel == channel)
        {
            v.gated = false;
            v.env.noteOff (p, fs);
        }
    }
}

void Engine::allNotesOff() noexcept
{
    const AmpParams p = ampParams();
    for (auto& v : voices)
    {
        if (v.isKeyDown())
        {
            v.gated = false;
            v.env.noteOff (p, fs);
        }
    }
}

void Engine::allSoundOff() noexcept
{
    for (int k = 0; k < kVoiceSlots; ++k)
        if (voices[k].isCounted())
            forceReleaseVoice (k);
}

void Engine::forceReleaseVoice (int k) noexcept
{
    Voice& v = voices[k];
    if (v.isFree() || v.isStolen())
        return;
    double ms = kStealMs;
   #if OGI_TEST_HOOKS
    if (hooks.stealRampZero)
        ms = 0.0;
   #endif
    v.state = Voice::State::stolen;
    v.gated = false;
    v.env.forceRelease (fs, ms);
    quotasDirty = true;
    writeVoiceHooks (k);
}

void Engine::stealVoice (int k) noexcept
{
   #if OGI_TEST_HOOKS
    ++hooks.voiceSteals;
    hooks.stolenSerials[hooks.stolenCount % EngineTestHooks::kStolenCapacity] = voices[k].serial;
    ++hooks.stolenCount;
   #endif
    forceReleaseVoice (k);
}

void Engine::endVoice (int k) noexcept
{
    // Its grains are silent (env 0): released with no fade.
    const VoiceGrain* const base = pool.getGrains().data();
    for (int i = 0; i < GiPool::kNumSlots; ++i)
    {
        if (pool.isActive (i) && base[i].voice == k)
        {
            pool.release (base[i]);
            spawner.clearPendingKill (i);
        }
    }
    voices[k] = Voice {};
    for (int s = 0; s < kGiStreams; ++s)
        quotaM[k][s] = kQuotaUncapped;
    quotasDirty = true;
    writeVoiceHooks (k);
}

bool Engine::endQuietestStolen() noexcept
{
    const int k = VoiceAllocator::quietestStolen (voices);
    if (k < 0)
        return false;
    endVoice (k);
    return true;
}

//==============================================================================
// Audio
void Engine::runSegment (int c0, int c1, std::int64_t t0) noexcept
{
    const int len = c1 - c0;
    if (len <= 0)
        return;

    // 1. Amp envelopes for the segment + the samples at which voices end.
    const AmpParams ap = ampParams();
    int numEnds = 0;
    for (int k = 0; k < kVoiceSlots; ++k)
    {
        Voice& v = voices[k];
        float* a = envBuf[k] + c0;
        if (v.isFree())
        {
            std::fill (a, a + len, 0.0f);
            continue;
        }
        const int e = v.env.process (a, len, ap, fs);
        if (e >= 0)
            ends[numEnds++] = { k, e, v.serial };
    }
    for (int i = 1; i < numEnds; ++i)
        for (int j = i; j > 0 && (ends[j].offset < ends[j - 1].offset
                                  || (ends[j].offset == ends[j - 1].offset && ends[j].serial < ends[j - 1].serial)); --j)
            std::swap (ends[j], ends[j - 1]);

    // 2. Schedulers (Free) of the counted voices × enabled streams. Pairs are
    //    filled in (serial ascending, stream ascending) order, then a STABLE
    //    insertion sort on offset alone: ties land in (offset, serial, stream)
    //    order and the voice slot never enters the key (V3).
    int order[kVoiceSlots] {};
    const int nv = VoiceAllocator::countedOldestFirst (voices, order);
    int numOnsets = 0;
    for (int i = 0; i < nv; ++i)
    {
        const int k = order[i];
        Voice& v = voices[k];
        for (int s = 0; s < kGiStreams; ++s)
        {
            if (! gp.s[s].enable)
                continue;
            const double T = periodT[k][s];
            const double J = jit[s];
            const std::uint64_t serial = v.serial, ep = v.epoch[s];
            const auto res = v.sched[s].process (len, onsetBuf[k][s], kOnsetCap,
                [this, T, J, s, serial, ep] (std::uint64_t idx) noexcept { return jitterGap (T, J, s, serial, ep, idx); });
            for (int j = 0; j < res.count; ++j)
            {
                const auto& o = onsetBuf[k][s][j];
                merged[numOnsets++] = { k, s, o.offset, o.frac, o.index, ep + o.index, serial };
            }
        }
    }
    for (int i = 1; i < numOnsets; ++i)
        for (int j = i; j > 0 && merged[j].offset < merged[j - 1].offset; --j)
            std::swap (merged[j], merged[j - 1]);

    // 3. Grain-major render in sub-segments split at voice ends and onsets
    //    (ends first at a shared offset: they free pool slots before spawns).
    int cur = 0, ie = 0, io = 0;
    while (ie < numEnds || io < numOnsets)
    {
        const int nextE = ie < numEnds ? ends[ie].offset : INT_MAX;
        const int nextO = io < numOnsets ? merged[io].offset : INT_MAX;
        const int at = std::min (nextE, nextO);
        if (at > cur)
        {
            renderGrains (c0 + cur, c0 + at, t0 + cur);
            cur = at;
        }
        while (ie < numEnds && ends[ie].offset == at)
        {
            const auto& e = ends[ie++];
            if (! voices[e.voice].isFree() && voices[e.voice].serial == e.serial)
                endVoice (e.voice);
        }
        while (io < numOnsets && merged[io].offset == at)
        {
            spawn (merged[io], t0 + at);
            ++io;
        }
    }
    if (len > cur)
        renderGrains (c0 + cur, c1, t0 + cur);
}

void Engine::renderGrains (int c0, int c1, std::int64_t t0) noexcept
{
    if (c1 <= c0)
        return;
    bool forceBank = false;
   #if OGI_TEST_HOOKS
    forceBank = hooks.forceBankPath;
   #endif
    const VoiceGrain* const base = pool.getGrains().data();
   #if OGI_TEST_HOOKS
    int killedSlots[GiPool::kNumSlots];
    int numKilled = 0;
   #endif
    pool.render ([this, base, c0, c1, t0, forceBank
                 #if OGI_TEST_HOOKS
                  , &killedSlots, &numKilled
                 #endif
                 ] (VoiceGrain& vg, GiPool::Fader& fader) noexcept
    {
        spawner.applyPendingKill (vg, base);
        GiFxRenderFlags ff;
       #if OGI_TEST_HOOKS
        const int slot = static_cast<int> (&vg - base);
        Grain& g = vg.g;
        ff.nanKillOff  = hooks.nanKillOff;
        ff.fxOrderSwap = hooks.fxOrderSwap;
        {
            auto& inj = hooks.injectNan;
            if (inj.slot == slot && ! inj.done && t0 >= inj.atSample && (g.fxActive & kFxSvf) != 0)
            {
                float* const f[4] { &g.svfIc1[0], &g.svfIc1[1], &g.svfIc2[0], &g.svfIc2[1] };
                *f[std::clamp (inj.field, 0, 3)] = inj.value;
                inj.done = true;
            }
            auto& fr = hooks.forceRetire;
            if (fr.slot == slot && ! fr.done && t0 >= fr.atSample && (g.fxActive & kFxSvf) != 0)
            {
                ff.forceKill = true;
                fr.done = true;
            }
        }
       #endif
        const bool keep = GiRenderer::renderGrain (vg, fader, bus, c0, c1, t0, xfade, morphBank, ff, forceBank);
       #if OGI_TEST_HOOKS
        if (ff.nanKilled)
        {
            ++hooks.nanKills;
            killedSlots[numKilled++] = slot;
        }
        if (! keep || fader.isSilent())
            fxstate::copy (hooks.fxShadow[slot], vg.g);   // retire shadow (fxNoReset control)
       #endif
        return keep;
    });
    spawner.pendingKillsApplied();
   #if OGI_TEST_HOOKS
    for (int k = 0; k < numKilled; ++k)
        if (pool.isActive (killedSlots[k]))
            ++hooks.nanKillNotRetired;
   #endif
}

void Engine::spawn (const PendingOnset& o, std::int64_t tAbs) noexcept
{
    Voice& v = voices[o.voice];
    if (! v.isCounted() || v.serial != o.serial)
        return;   // ended / stolen inside this segment

    const int s = std::clamp (o.stream, 0, kGiStreams - 1);
    const auto& sp = gp.s[s];
    SlotTape* tp = current[(size_t) s];   // stream s reads slot s
    if (! sp.enable || tp == nullptr || tp->frames <= 0)
        return;

    // Draws BEFORE acquire (parent rule): a refused or stolen spawn consumes the
    // same draws. Keyed on the note serial and the per-(k,s) counter index
    // epoch + onsetIndex (PD13), never the voice slot. Every draw goes through
    // its fixed Slot; a randomisation at 0 draws nothing.
    const std::uint64_t counter = (v.serial << 24) | (o.idx & 0xFFFFFFull);
    const ouaricon::grain::CounterRng::Draw draw (seed, kGrainStream + static_cast<std::uint64_t> (s), counter);
    auto uni = [&draw] (std::uint32_t slot) noexcept { return static_cast<double> (draw.nextFloat (slot)); };

    // Length: size · (1 + sizeRand · (2u − 1) · 0.5), clamped [5, 2000] ms.
    double sizeMs = static_cast<double> (sp.sizeMs);
    if (sp.sizeRand > 0.0f)
        sizeMs *= 1.0 + static_cast<double> (sp.sizeRand) * (2.0 * static_cast<double> (draw.nextFloat (Slot::Size)) - 1.0) * 0.5;
    const double len = std::clamp (sizeMs, kMinSizeMs, kMaxSizeMs) * 0.001 * fs;

    // Pitch (parent Spawner.h:182-204 + the key term): st = κ·key + pitch, then
    //   scale Off: + ρ(2u − 1) (Slot::Pitch), pitchCents the same draw ·100;
    //   scale on:  + pick/100, pick = ActiveSet::pick over [lo, hi) when ρ > 0
    //              and hi > lo (P13), else no draw;
    // then the ±48 clamp; rate = 2^(st/12) · tapeRate / fs.
    float pitchCents = 0.0f;
    double st = voiceSemis (v, s);
    const PitchSel& sel = scale.selection (s);
    if (sel.set == nullptr)
    {
        if (sp.pitchRand > 0.0f)
        {
            st += static_cast<double> (sp.pitchRand) * (2.0 * uni (Slot::Pitch) - 1.0);
            pitchCents = static_cast<float> (static_cast<double> (sp.pitchRand) * (2.0 * uni (Slot::Pitch) - 1.0) * 100.0);
        }
    }
    else if (sp.pitchRand > 0.0f && sel.hi > sel.lo)
    {
        pitchCents = sel.set->pick (draw, sel.lo, sel.hi);
        st += static_cast<double> (pitchCents) / 100.0;
    }
    st = std::clamp (st, -kMaxSemis, kMaxSemis);
    const double rate  = std::exp2 (st / 12.0) * tp->rate / fs;
    const int    level = ouaricon::grain::latchLevel (rate, kTapeLevels);   // ALWAYS 4 (P6)
    const int    set   = ouaricon::grain::SincBank::setFor (std::ldexp (rate, -level));

    // Direction: rp >= 1 → reverse, rp > 0 → u < rp (0 % / 100 % exact, no draw).
    int dir = 1;
    const double rp = static_cast<double> (sp.reverseProb);
    if (rp >= 1.0)
        dir = -1;
    else if (rp > 0.0)
        dir = uni (Slot::Reverse) < rp ? -1 : 1;

    // Position: scan centre c, spray c + s·W·(u − ½), region fit (no
    // record-head clamp): forward [lo, hi − reach], reverse [lo + reach, hi] (P20).
    const LoopRegion& rg = region[s];
    const double W = rg.width();
    if (! (W > 0.0))
        return;
    const double sigmaSec = (sp.scanMode == kScanFreeRun) ? freeRun[s].at (tAbs) : v.scan[s].at (tAbs);
    const double c = scanCentre (rg, static_cast<double> (sp.position), sigmaSec * tp->rate, sp.scanMode);
    double x = c;
    if (sp.spray > 0.0f)
        x += static_cast<double> (sp.spray) * W * (uni (Slot::PosSpray) - 0.5);
    const double reach = rate * len;
    double p0 = x, wLo = 0.0, wW = 0.0;
    if (reach < W)
    {
        p0 = dir > 0 ? reflectInto (x, rg.lo, rg.hi - reach)
                     : reflectInto (x, rg.lo + reach, rg.hi);
    }
    else
    {
        // reach >= W: the read wraps inside the region (documented sampler behaviour).
        p0  = std::clamp (x, rg.lo, rg.hi);
        if (p0 >= rg.hi) p0 = rg.lo;
        wLo = rg.lo;
        wW  = W;
    }

    // Stereo spread (constant-power balance about centre; the stream pan stays
    // on the bus) and gain rand (cut only): parent Spawner.h:247-263 verbatim.
    float gainL = 1.0f, gainR = 1.0f;
    if (sp.spread > 0.0f)
    {
        const float px = static_cast<float> (static_cast<double> (sp.spread) * (2.0 * uni (Slot::Pan) - 1.0));
        const auto  g = panGains (px);
        gainL = g.a;
        gainR = g.b;
    }
    if (sp.gainRand > 0.0f)
    {
        const float g = static_cast<float> (std::pow (10.0, -static_cast<double> (sp.gainRand) * uni (Slot::Gain)
                                                            * kGainRandDb / 20.0));
        gainL *= g;
        gainR *= g;
    }

    // Window morph step + skew tilt, latched (P13).
    const int  windowStep = WindowMorphBank::stepFor (sp.window);
    const auto tilt       = ouaricon::grain::WindowLut::makeTilt (0.5f + 0.45f * sp.windowSkew);

    // Per-grain FX latch (PLAN-2.3 T5): the parent planFx, before acquire, so a
    // refused or stolen spawn consumes the same draws (FxCutoff / FxCrushProb /
    // FxRing are keyed by slot: no 2.2 draw moves).
    FxPlan fxp;
    planFx (fxp, sp, fs, uni);
   #if OGI_TEST_HOOKS
    if (hooks.legacySvfMake && (fxp.fxActive & kFxSvf) != 0)
        fxp.svfMake = 1.0f;   // PFc6: the pre-D10 latch
    if (hooks.snapshotSteals)
        for (int j = 0; j < kPoolLive; ++j)
            hooks.stealSnap[(size_t) j] = pool.getGrains()[(size_t) j];
   #endif

    bool stolenFlags[kVoiceSlots];
    for (int k = 0; k < kVoiceSlots; ++k)
        stolenFlags[k] = voices[k].isStolen();

    AcquirePolicy policy;
   #if OGI_TEST_HOOKS
    policy.overQuotaOff = hooks.overQuotaOff;
    policy.d25Off       = hooks.d25Off;
   #endif
    const auto a = spawner.acquire (pool, gp.grainLimit, quotaM, stolenFlags, o.voice, s, policy);
    if (a.grain == nullptr)
        return;

    VoiceGrain& vg = *a.grain;
    Grain& g = vg.g;
    g.stream     = s;
    g.level      = level;
    g.set        = set;
    g.dir        = dir;
    g.age        = 0;
    g.rate       = rate;
    g.step       = static_cast<double> (dir) * rate;
    g.frac       = o.frac;
    g.len        = len;
    g.invLen     = 1.0 / len;
    g.startDelay = 0.0;
    g.readAbs    = p0 + g.step * o.frac;
    if (wW > 0.0 && g.readAbs >= wLo + wW)
        g.readAbs -= wW;
    else if (wW > 0.0 && g.readAbs < wLo)
        g.readAbs += wW;
    g.gainL      = gainL;
    g.gainR      = gainR;
    g.windowStep = windowStep;
    g.windowSkew = sp.windowSkew;
    g.tiltT      = tilt.t;
    g.tiltA      = tilt.a;
    g.tiltB      = tilt.b;
    g.pitchCents = pitchCents;
    assignFxLatch (g, fxp);
    // killGain 1, killStep 0 and every FX STATE field from value-initialisation;
    // the FX LATCH from assignFxLatch.

    vg.voice  = o.voice;
    vg.serial = v.serial;
    vg.tape   = tp;
    vg.slot   = s;
    vg.wrapLo = wLo;
    vg.wrapW  = wW;

   #if OGI_TEST_HOOKS
    if (hooks.fxSharedLatch)
    {
        // FV2–FV4 control: the renderer gets a latch WITHOUT the per-grain draws;
        // the log below keeps the real ones.
        auto sp2 = sp;
        sp2.filterRand = 0.0f;
        sp2.ringRand   = 0.0f;
        sp2.crushProb  = 1.0f;
        FxPlan q;
        planFx (q, sp2, fs, uni);
        if (hooks.legacySvfMake && (q.fxActive & kFxSvf) != 0)
            q.svfMake = 1.0f;
        assignFxLatch (g, q);
    }
    if (hooks.fxNoReset)
        fxstate::copy (g, hooks.fxShadow[a.slot]);
    {
        const Grain fresh {};
        if (! fxstate::stateEq (g, fresh))
            ++hooks.fxStateNonDefaultAtSpawn;
    }
    if (hooks.snapshotSteals && a.stolen)
    {
        VoiceGrain snap = hooks.stealSnap[(size_t) std::clamp (a.slot, 0, kPoolLive - 1)];
        if (hooks.stealSnapPerturb)
            snap.g.svfIc1[0] = std::nextafter (snap.g.svfIc1[0], 2.0f);
        const auto& gs = pool.getGrains();
        for (int j = kPoolLive; j < GiPool::kNumSlots; ++j)
        {
            if (! pool.isActive (j))
                continue;
            const VoiceGrain& f = gs[(size_t) j];
            if (f.serial == snap.serial && f.g.age == snap.g.age && juce::exactlyEqual (f.g.readAbs, snap.g.readAbs))
            {
                ++hooks.stealStateChecked;
                if (! (fxstate::stateEq (f.g, snap.g) && fxstate::latchEq (f.g, snap.g)))
                    ++hooks.stealStateMismatches;
                break;
            }
        }
    }
    ++hooks.spawns;
    if (a.killed)  ++hooks.r3Kills;
    if (a.stolen)  ++hooks.nativeSteals;
    if (a.capKill) ++hooks.capKills;
    if (s == 0)
    {
        hooks.voiceRate[o.voice]  = rate;
        hooks.voiceLevel[o.voice] = level;
    }
    if (! hooks.log.empty())
    {
        auto& entry = hooks.log[static_cast<std::size_t> (hooks.logCount % static_cast<std::uint64_t> (hooks.log.size()))];
        entry.serial      = v.serial;
        entry.onsetIndex  = o.index;
        entry.onsetSample = tAbs;
        entry.frac        = o.frac;
        entry.len         = len;
        entry.startPos    = p0;
        entry.rate        = rate;
        entry.voice       = o.voice;
        entry.level       = level;
        entry.poolSlot    = a.slot;
        entry.stream      = s;
        entry.dir         = dir;
        entry.gainL       = gainL;
        entry.gainR       = gainR;
        entry.windowStep  = windowStep;
        entry.tiltT       = tilt.t;
        entry.pitchCents  = pitchCents;
        entry.centre      = c;
        entry.regionW     = W;
        entry.counterIdx  = o.idx;
        entry.fxActive    = fxp.fxActive;
        entry.svfMode     = fxp.svfMode;
        entry.fcHz        = fxp.fcHz;
        entry.ringHz      = fxp.ringHz;
        entry.crushed     = fxp.crushed;
        entry.driveD      = fxp.driveD;
        ++hooks.logCount;
    }
   #endif
}

void Engine::mixPiece (int c0, int c1, float* outL, float* outR) noexcept
{
    // PD12. A stream whose audibility ramp sits at exactly 0 for the whole cell
    // is skipped (never added as +0.0f: −0 + 0 = +0 would break the 2.1 hashes).
    // Stream 0's accumulator is always formed: when no stream is audible its
    // term is the output (the 2.1 arithmetic, sign of zero included).
    bool audible[kGiStreams];
    for (int s = 0; s < kGiStreams; ++s)
        audible[s] = ! audibleRamp[s].settledAt (0.0f);

    for (int s = 0; s < kGiStreams; ++s)
    {
        if (! audible[s] && s != 0)
        {
            for (int k = 0; k < kVoiceSlots; ++k)
            {
                std::fill (bus.s[k][s][0] + c0, bus.s[k][s][0] + c1, 0.0f);
                std::fill (bus.s[k][s][1] + c0, bus.s[k][s][1] + c1, 0.0f);
            }
            continue;
        }
        // acc_s = Σ_k (env_k · lawGain_kS) · bus_kS, per sample in ascending k
        // from 0.0f (the 2.1 accumulation order); the bus is cleared after reading.
        float* aL = accL[s];
        float* aR = accR[s];
        for (int c = c0; c < c1; ++c)
        {
            aL[c] = 0.0f;
            aR[c] = 0.0f;
        }
        for (int k = 0; k < kVoiceSlots; ++k)
        {
            float* bl = bus.s[k][s][0];
            float* br = bus.s[k][s][1];
            const CellLerp& lg = lawGain[k][s];
            for (int c = c0; c < c1; ++c)
            {
                const float a = envBuf[k][c] * lg.at (c);
                if (a > 0.0f)
                {
                    aL[c] += a * bl[c];
                    aR[c] += a * br[c];
                }
                bl[c] = 0.0f;
                br[c] = 0.0f;
            }
        }
    }

    const float vgain = voiceGainNow;
    for (int c = c0; c < c1; ++c)
    {
        // Per stream level / audibility / pan, voice headroom; reverb absent (unity).
        float yL = 0.0f, yR = 0.0f;
        bool have = false;
        for (int s = 0; s < kGiStreams; ++s)
        {
            if (! audible[s])
                continue;
            const float lv = levelRamp[s].value (c) * audibleRamp[s].value (c) * vgain;
            const float tL = accL[s][c] * lv * panLRamp[s].value (c);
            const float tR = accR[s][c] * lv * panRRamp[s].value (c);
            if (! have)
            {
                yL = tL;
                yR = tR;
                have = true;
            }
            else
            {
                yL += tL;
                yR += tR;
            }
        }
        if (! have)
        {
            const float lv = levelRamp[0].value (c) * audibleRamp[0].value (c) * vgain;
            yL = accL[0][c] * lv * panLRamp[0].value (c);
            yR = accR[0][c] * lv * panRRamp[0].value (c);
        }

        // Wet safety = the parent D9 knee (PLAN-2.4 PD1; identity at or below
        // kKneeT), then output_gain.
       #if OGI_TEST_HOOKS
        if (hooks.legacyWetTanh)    // PLAN-2.4 PD1: the exact 2.1–2.3 statements
        {
            yL = kWetCeil * std::tanh (yL / kWetCeil);
            yR = kWetCeil * std::tanh (yR / kWetCeil);
        }
        else if (! hooks.wetSafetyOff)   // PF rows (PLAN-2.3 PD5): identity
       #endif
        {
            yL = wetKnee (yL);
            yR = wetKnee (yR);
        }
        const float og = outGainRamp.value (c);
        outL[c - c0] = yL * og;
        outR[c - c0] = yR * og;
    }
}
} // namespace ogi
