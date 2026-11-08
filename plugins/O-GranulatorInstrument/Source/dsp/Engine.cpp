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

namespace ogi
{
namespace
{
    bool sameBits (double a, double b) noexcept
    {
        return std::memcmp (&a, &b, sizeof (double)) == 0;
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
    const auto& sp = gp.s[0];
    outGainRamp.reset (dbToGain (gp.outputGainDb));
    levelRamp.reset (levelDbToGain (sp.levelDb));
    const auto pg = panGains (sp.pan);
    panLRamp.reset (pg.a);
    panRRamp.reset (pg.b);
    audibleRamp.reset ((sp.enable && ! sp.mute) ? 1.0f : 0.0f);

    updateRegion();
    freeRun.restart (0, static_cast<double> (sp.scanRate) / fs);

    quotas = QuotasN<kVoiceSlots> {};
    quotasDirty = true;
    prevCap = -1;
    prevLMax = prevDensity = -1.0;
    for (int k = 0; k < kVoiceSlots; ++k)
    {
        quotaM[k] = kQuotaUncapped;
        periodT[k] = uncappedPeriod();
        prevDemand[k] = 0.0;
        lawTarget[k] = 1.0f;
        lawGain[k].reset (1.0f);
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

double Engine::voiceRate (const Voice& v, const SlotTape& tape) const noexcept
{
    // ARCHITECTURE Algorithm §2: st = κ·(note + bend − root − fine/100) + pitch,
    // clamped to ±48; rate = 2^(st/12) · tapeRate / fs (double).
    const auto& sp = gp.s[0];
    const double key = static_cast<double> (v.note) - static_cast<double> (sp.rootNote)
                     - static_cast<double> (sp.rootFineCt) / 100.0
                     + static_cast<double> (bend) * static_cast<double> (gp.pitchBendRangeSt);
    const double st0 = static_cast<double> (sp.keyTrack) * key + static_cast<double> (sp.pitchSt);
    const double st  = std::clamp (st0, -kMaxSemis, kMaxSemis);
    return std::exp2 (st / 12.0) * tape.rate / fs;
}

double Engine::rateForNote (int note) const noexcept
{
    const SlotTape* tp = current[0];
    if (tp == nullptr)
        return 0.0;
    Voice v;
    v.note = note;
    return voiceRate (v, *tp);
}

double Engine::uncappedPeriod() const noexcept
{
    return thinnedPeriod (fs, static_cast<double> (gp.s[0].density), lMaxSamples(), kQuotaUncapped);
}

double Engine::lMaxSamples() const noexcept
{
    const auto& sp = gp.s[0];
    const double ms = static_cast<double> (sp.sizeMs) * (1.0 + 0.5 * static_cast<double> (sp.sizeRand));
    return std::clamp (ms, kMinSizeMs, kMaxSizeMs) * 0.001 * fs;
}

double Engine::lenBarSamples() const noexcept
{
    return std::clamp (static_cast<double> (gp.s[0].sizeMs), kMinSizeMs, kMaxSizeMs) * 0.001 * fs;
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
        hooks.voiceRate[k]  = voiceRate (v, *current[0]);
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
   #if OGI_TEST_HOOKS
    if (hooks.seedOverrideOn)
        seed = hooks.seedOverride;
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
    const auto& sp = gp.s[0];

    outGainRamp.setTarget (dbToGain (gp.outputGainDb), rampCells20);
    outGainRamp.tick();
    levelRamp.setTarget (levelDbToGain (sp.levelDb), rampCells20);
    levelRamp.tick();
    const auto pg = panGains (sp.pan);
    panLRamp.setTarget (pg.a, rampCells20);
    panLRamp.tick();
    panRRamp.setTarget (pg.b, rampCells20);
    panRRamp.tick();
    audibleRamp.setTarget ((sp.enable && ! sp.mute) ? 1.0f : 0.0f, rampCells10);
    audibleRamp.tick();

    takeTapes (t);
    finishXfades (t);
    retireScan();
    updateRegion();
    enforcePolyphony();
    updateQuotas();
    updateScan (t);

   #if OGI_TEST_HOOKS
    ++hooks.gridTicks;
    hooks.liveNow   = spawner.countAlive (pool);
    hooks.liveMin   = std::min (hooks.liveMin, hooks.liveNow);
    hooks.liveMax   = std::max (hooks.liveMax, hooks.liveNow);
    hooks.activeNow = pool.countActive();
    spawner.countAlivePerVoice (pool, hooks.liveVoice);
    for (int k = 0; k < kVoiceSlots; ++k)
    {
        hooks.quotaM[k]  = quotaM[k];
        hooks.quotaQ[k]  = quotas.q[k];
        hooks.periodT[k] = periodT[k];
        writeVoiceHooks (k);
    }
    hooks.lambda   = quotas.lambda;
    hooks.capped   = quotas.capped;
    hooks.regionLo = region.lo;
    hooks.regionHi = region.hi;
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
    const SlotTape* tp = current[0];
    if (tp == nullptr)
    {
        region = LoopRegion {};
        return;
    }
    const auto& sp = gp.s[0];
    region = loopRegion (static_cast<double> (sp.loopStart), static_cast<double> (sp.loopEnd),
                         tp->frames, kMinLoopMs * 0.001 * tp->rate);
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

void Engine::updateQuotas() noexcept
{
    const auto& sp = gp.s[0];
    const bool canSpawn = sp.enable && current[0] != nullptr;
    const double n = streamDemand (static_cast<double> (sp.density), static_cast<double> (sp.sizeMs), fs);

    double demand[kVoiceSlots];
    for (int k = 0; k < kVoiceSlots; ++k)
        demand[k] = (canSpawn && voices[k].isCounted()) ? n : 0.0;

    bool thinOff = false, periodOff = false;
   #if OGI_TEST_HOOKS
    thinOff   = hooks.thinningOff;
    periodOff = hooks.periodThinOff;
   #endif
    const int thinMode = (thinOff ? 1 : 0) + (periodOff ? 2 : 0);

    const double lMax    = lMaxSamples();
    const double density = static_cast<double> (sp.density);
    const bool changed = quotasDirty || prevCap != gp.grainLimit || thinMode != prevThinOff
                      || ! sameBits (lMax, prevLMax) || ! sameBits (density, prevDensity)
                      || std::memcmp (demand, prevDemand, sizeof (demand)) != 0;
    if (! changed)
    {
        // Inputs unchanged: hold every law gain (a CellLerp needs a push per tick).
        for (int k = 0; k < kVoiceSlots; ++k)
            lawGain[k].push (lawTarget[k]);
        return;
    }

    quotas = waterFillN (demand, gp.grainLimit);
    std::memcpy (prevDemand, demand, sizeof (demand));
    prevCap = gp.grainLimit;
    prevLMax = lMax;
    prevDensity = density;
    prevThinOff = thinMode;
    quotasDirty = false;

    const double lenBar = lenBarSamples();
    for (int k = 0; k < kVoiceSlots; ++k)
    {
        const int m = thinOff ? kQuotaUncapped : quotas.m[k];
        quotaM[k] = m;
        const double T = thinnedPeriod (fs, density, lMax, periodOff ? kQuotaUncapped : m);
        Voice& v = voices[k];
        // R§1.3: a quota DECREASE re-times the pending gap at once (block-size
        // invariant: quotas change only at absolute grid ticks); an increase
        // lets the latched gap roll.
        if (v.isCounted() && T > periodT[k])
            v.sched.rescheduleNext (T);
        periodT[k] = T;
        lawTarget[k] = GiRenderer::voiceLawGain (effectiveCount (lenBar, T), morphBank);
        lawGain[k].push (lawTarget[k]);
    }
}

void Engine::updateScan (std::int64_t t) noexcept
{
    const auto& sp = gp.s[0];
    const double rateSec  = static_cast<double> (sp.scanRate) / fs;   // tape seconds per output sample
    const double tapeRate = current[0] != nullptr ? current[0]->rate : 0.0;
    freeRun.reanchor (t, rateSec, scanPeriodSec (region, tapeRate, kScanFreeRun));
    const double voicePeriod = scanPeriodSec (region, tapeRate, sp.scanMode);
    for (auto& v : voices)
        if (! v.isFree())
            v.scan.reanchor (t, rateSec, voicePeriod);
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
    v.sched.reset();                           // first onset exactly at the note-on sample
    v.scan.restart (t, static_cast<double> (gp.s[0].scanRate) / fs);

    // Until the next grid tick includes it, the new voice runs unthinned.
    quotaM[k]  = kQuotaUncapped;
    periodT[k] = uncappedPeriod();
    const double n = streamDemand (static_cast<double> (gp.s[0].density), static_cast<double> (gp.s[0].sizeMs), fs);
    lawTarget[k] = GiRenderer::voiceLawGain (n, morphBank);
    lawGain[k].reset (lawTarget[k]);
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
    quotaM[k] = kQuotaUncapped;
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

    // 2. Schedulers (Free) of the counted voices; merge by (offset, serial).
    int numOnsets = 0;
    for (int k = 0; k < kVoiceSlots; ++k)
    {
        Voice& v = voices[k];
        if (! v.isCounted())
            continue;
        const double T = periodT[k];
        const auto res = v.sched.process (len, onsetBuf[k], kOnsetCap, [T] (std::uint64_t) noexcept { return T; });
        for (int i = 0; i < res.count; ++i)
        {
            const auto& o = onsetBuf[k][i];
            merged[numOnsets++] = { k, o.offset, o.frac, o.index, v.serial };
        }
    }
    for (int i = 1; i < numOnsets; ++i)
        for (int j = i; j > 0 && (merged[j].offset < merged[j - 1].offset
                                  || (merged[j].offset == merged[j - 1].offset && merged[j].serial < merged[j - 1].serial)); --j)
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
    const VoiceGrain* const base = pool.getGrains().data();
    pool.render ([this, base, c0, c1, t0] (VoiceGrain& vg, GiPool::Fader& fader) noexcept
    {
        spawner.applyPendingKill (vg, base);
        return GiRenderer::renderGrain (vg, fader, bus, c0, c1, t0, xfade);
    });
    spawner.pendingKillsApplied();
}

void Engine::spawn (const PendingOnset& o, std::int64_t tAbs) noexcept
{
    Voice& v = voices[o.voice];
    if (! v.isCounted() || v.serial != o.serial)
        return;   // ended / stolen inside this segment

    const auto& sp = gp.s[0];
    SlotTape* tp = current[0];
    if (! sp.enable || tp == nullptr || tp->frames <= 0)
        return;

    // Draws BEFORE acquire (parent rule): a refused or stolen spawn consumes the
    // same draws. Keyed on the note serial, never the voice slot.
    const std::uint64_t counter = (v.serial << 24) | (o.index & 0xFFFFFFull);
    const ouaricon::grain::CounterRng::Draw draw (seed, kGrainStream + 0ull, counter);

    double sizeMs = static_cast<double> (sp.sizeMs);
    if (sp.sizeRand > 0.0f)
        sizeMs *= 1.0 + static_cast<double> (sp.sizeRand) * (2.0 * static_cast<double> (draw.nextFloat (Slot::Size)) - 1.0) * 0.5;
    const double len = std::clamp (sizeMs, kMinSizeMs, kMaxSizeMs) * 0.001 * fs;

    const double rate  = voiceRate (v, *tp);
    const int    level = ouaricon::grain::latchLevel (rate, kTapeLevels);   // ALWAYS 4 (P6)
    const int    set   = ouaricon::grain::SincBank::setFor (std::ldexp (rate, -level));

    // Region fit (no record-head clamp in 2.1).
    const double W = region.width();
    if (! (W > 0.0))
        return;
    const double sigmaSec = (sp.scanMode == kScanFreeRun) ? freeRun.at (tAbs) : v.scan.at (tAbs);
    const double c = scanCentre (region, static_cast<double> (sp.position), sigmaSec * tp->rate, sp.scanMode);
    const double reach = rate * len;
    double p0 = c, wLo = 0.0, wW = 0.0;
    if (reach < W)
    {
        p0 = reflectInto (c, region.lo, region.hi - reach);
    }
    else
    {
        // reach >= W: the read wraps inside the region (documented sampler behaviour).
        p0  = std::clamp (c, region.lo, region.hi);
        if (p0 >= region.hi) p0 = region.lo;
        wLo = region.lo;
        wW  = W;
    }

    bool stolenFlags[kVoiceSlots];
    for (int k = 0; k < kVoiceSlots; ++k)
        stolenFlags[k] = voices[k].isStolen();

    AcquirePolicy policy;
   #if OGI_TEST_HOOKS
    policy.overQuotaOff = hooks.overQuotaOff;
    policy.d25Off       = hooks.d25Off;
   #endif
    const auto a = spawner.acquire (pool, gp.grainLimit, quotaM, stolenFlags, o.voice, policy);
    if (a.grain == nullptr)
        return;

    VoiceGrain& vg = *a.grain;
    Grain& g = vg.g;
    g.stream     = 0;
    g.level      = level;
    g.set        = set;
    g.dir        = 1;
    g.age        = 0;
    g.rate       = rate;
    g.step       = rate;
    g.frac       = o.frac;
    g.len        = len;
    g.invLen     = 1.0 / len;
    g.startDelay = 0.0;
    g.readAbs    = p0 + rate * o.frac;
    if (wW > 0.0 && g.readAbs >= wLo + wW)
        g.readAbs -= wW;
    g.gainL = 1.0f;
    g.gainR = 1.0f;
    // killGain = 1, killStep = 0 and every FX / window field from value-initialisation.

    vg.voice  = o.voice;
    vg.serial = v.serial;
    vg.tape   = tp;
    vg.slot   = 0;
    vg.wrapLo = wLo;
    vg.wrapW  = wW;

   #if OGI_TEST_HOOKS
    ++hooks.spawns;
    if (a.killed)  ++hooks.r3Kills;
    if (a.stolen)  ++hooks.nativeSteals;
    if (a.capKill) ++hooks.capKills;
    hooks.voiceRate[o.voice]  = rate;
    hooks.voiceLevel[o.voice] = level;
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
        ++hooks.logCount;
    }
   #endif
}

void Engine::mixPiece (int c0, int c1, float* outL, float* outR) noexcept
{
    for (int c = c0; c < c1; ++c)
    {
        float sL = 0.0f, sR = 0.0f;
        for (int k = 0; k < kVoiceSlots; ++k)
        {
            const float a = envBuf[k][c] * lawGain[k].at (c);
            if (a > 0.0f)
            {
                sL += a * bus.s[k][0][c];
                sR += a * bus.s[k][1][c];
            }
            bus.s[k][0][c] = 0.0f;
            bus.s[k][1][c] = 0.0f;
        }

        // Stream 1 level / audibility / pan, voice headroom; reverb absent (unity).
        const float lv = levelRamp.value (c) * audibleRamp.value (c) * voiceGainLin;
        float yL = sL * lv * panLRamp.value (c);
        float yR = sR * lv * panRRamp.value (c);

        // P8 wet safety, then output_gain.
        yL = kWetCeil * std::tanh (yL / kWetCeil);
        yR = kWetCeil * std::tanh (yR / kWetCeil);
        const float og = outGainRamp.value (c);
        outL[c - c0] = yL * og;
        outR[c - c0] = yR * og;
    }
}
} // namespace ogi
