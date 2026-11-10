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
    // PLAN-2.4 PD11: the reverb reads engine-owned atomics (bound once).
    reverb.bind (&revEnabled, &revSize, &revDamp, &revPredelay, &revMix, &revMod, &revShimmer);
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
    recorder.bind (apvts);   // PLAN-2.6 PD18: rec_* as raw atomics (no ControlGrid edit)
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
    nowT = 0;
    noteStack.clear();
    sustainDown = false;
    detached = false;
    lastKey = 0.0;
    lastKeySet = false;
    lastKeyNoteId = -1;
    lastKeyNote = 0;
    mpe.reset();   // PLAN-2.5b PD7
    tracker.prepare (fs);   // 2.5a: virtual 120 bpm until the first host block
    recorder.prepare (fs);  // PLAN-2.6 PD9: a take in flight commits at the old rate; buffer grow-only

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

    // PLAN-2.4 PD3 / PD7: the fixed z^-32 returns zeroed; the chains prepared
    // (lengths, AGC coefficients, state cleared; no allocation).
    std::memset (fbRet, 0, sizeof (fbRet));
    for (auto& c : chain)
        c.prepare (fs);

    rampCells20 = rampCellsFor (fs);
    rampCells10 = rampCellsFor (fs, kMixerRampMs);

    gp.latch (ptrs);
    prevVoiceMode = gp.voiceMode;   // PLAN-2.5b T7: no D32 switch at the first tick
    outGainRamp.reset (dbToGain (gp.outputGainDb));
    setStreamRamps (true);

    // 2.5a sources (mirrors 82406fce Engine.cpp:90-101, P4): LFO phases ramp
    // from the latched phase, accumulators at 0, macros snapped (no fade at
    // load), the matrix slots snapped to the latched routes.
    rampCellsMod = rampCellsFor (fs, kModRampMs);
    for (int n = 0; n < kGiLfos; ++n)
    {
        lfos[n].prepare (fs, rampCellsMod, gp.lfo[n].phaseDeg);
        lfoRateHz[n]  = static_cast<double> (gp.lfo[n].rateHz);
        lfoRateOff[n] = 0.0f;
        lfoXf[n].reset (gp.lfoMode[n] != 0 ? 1.0f : 0.0f);
    }
    for (int n = 0; n < kGiMacros; ++n)
        macroRamp[n].reset (gp.macro[n]);
    std::fill (std::begin (srcG), std::end (srcG), 0.0f);
    wheelMsb = wheelLsb = 0;
    chanPressure = 0.0f;
    chanSlide = 0.0f;
    for (auto& m : vm)
        m.clear();
    matrix.prepare (rampCellsMod, gp);
    std::fill (std::begin (rawBase), std::end (rawBase), 0.0f);
    for (int s = 0; s < kGiStreams; ++s)
    {
        streamRoutedG[s] = false;
        levelNow[s] = levelRamp[s].current();   levelMod[s].reset (levelNow[s]);   useLevelMod[s] = false;
        panValRamp[s].reset (gp.s[s].pan);
        panLNow[s] = panLRamp[s].current();     panLMod[s].reset (panLNow[s]);
        panRNow[s] = panRRamp[s].current();     panRMod[s].reset (panRNow[s]);
        usePanMod[s] = false;
    }
    outGainNow = outGainRamp.current();  outGainMod.reset (outGainNow);  useOutMod = false;
    for (auto& x : pairMix) x = false;
    numPairsRouted = numPairsRoutedPrev = 0;
    prevTempo = gp.syncMode == 1;
    for (int k = 0; k < kVoiceSlots; ++k)
        for (int s = 0; s < kGiStreams; ++s)
        {
            tclk[k][s].restart();
            burstPending[k][s] = false;
        }
    if (prevTempo)
        applyTempoEffective();
    // PD8: the held mono values start at 0; the first note ramps from 0.
    std::fill (std::begin (monoFrom), std::end (monoFrom), 0.0f);
    std::fill (std::begin (monoVal), std::end (monoVal), 0.0f);
    newestW.reset (1.0f);
    newestSer = 0;
    for (auto& a : destTel) a.store (0.0f, std::memory_order_relaxed);
    for (auto& a : srcTel) a.store (0.0f, std::memory_order_relaxed);
    for (auto& a : lfoPhaseTel) a.store (0.0f, std::memory_order_relaxed);

    // PLAN-2.4 PD11: reverb (allocates: host thread), atomics from the params.
    writeReverbAtomics();
    reverb.prepare (fs, kCell);
    revMixWasOn = gp.reverbMix > 0.0f;

    // PLAN-2.4 PD7: chain controls snapped to the params; the knob is UNITY
    // (reset once, never ticked: value (c) == 1.0f exactly, P1).
    unityKnob.reset (1.0f);
    parityKnob.reset (kFbKnob * gp.feedback);
    diffRamp.reset (gp.fbDiffusion);
    satDRamp.reset (1.0f + kSatSpan * gp.fbSaturation);
    fbLowLogRamp.reset (std::log2 (gp.fbLowCutHz));
    fbHighLogRamp.reset (std::log2 (gp.fbHighCutHz));
    chainLowHz = chainHighHz = 0.0f;
    updateChainCutoffs();
    for (int s = 0; s < kGiStreams; ++s)
    {
        prevEnable[s] = gp.s[s].enable;
        jit[s] = jitterOf (s);
    }

    updateRegion();
    for (int s = 0; s < kGiStreams; ++s)
        freeRun[s].restart (0, static_cast<double> (gp.s[s].scanRate) / fs);

    // PLAN-2.4 PD4 / PD5: heads parked at their region start (a saved freeze On
    // stays parked: run starts at 0), λ ramp snapped to the feedback param.
    fbRamp.reset (gp.feedback);
    fbNow = fbRamp.current();   fbMod.reset (fbNow);     useFbMod = false;
    diffNow = diffRamp.current(); diffMod.reset (diffNow); useDiffMod = false;
    satDNow = satDRamp.current(); satDMod.reset (satDNow); useSatMod = false;
    prevFreeze = gp.freeze;
    pkRel = static_cast<float> (std::exp (-static_cast<double> (kCell) / (0.05 * fs)));
    for (int s = 0; s < kTapeSlots; ++s)
    {
        auto& hd = head[s];
        hd.run.reset (0.0f);
        hd.parked = true;
        hd.swapWaiting = false;
        hd.returnPk = 0.0f;
        hd.wrapOff = -1;
        const SlotTape* tp = current[(size_t) s];
        if (tp != nullptr)
        {
            hd.relatch (region[s].lo, region[s].hi, tp->frames);
            headRelatch[s] = false;
        }
        else
        {
            hd.h = hd.lo = hd.cellStart = 0;
            hd.hi = 1;
            headRelatch[s] = true;
        }
    }

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
            gFbTarget[k][s] = 1.0f;
            gFbLerp[k][s].reset (1.0f);
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

double Engine::voiceSemis (const Voice& v, const GiGridParams::Stream& sp, std::int64_t t) const noexcept
{
    // ARCHITECTURE Algorithm §2: st = κ·(note + bend − root − fine/100) + pitch
    // (before the random term and the ±48 clamp).
    double key = static_cast<double> (v.note) - static_cast<double> (sp.rootNote)
               - static_cast<double> (sp.rootFineCt) / 100.0
               + static_cast<double> (bend) * static_cast<double> (gp.pitchBendRangeSt);
    // PLAN-2.5b PD11 [correction ARCH §10 term order]: the 2.5b terms are
    // APPENDED — glide g(t), MPE member bend · memberRange, VST3 NE cents —
    // and an exactly-zero term is not added, so the 2.5a key stays bitwise.
    const double g = v.glideAt (t);
    if (g > 0.0 || g < 0.0)                     key += g;
    if (v.mpeSemis > 0.0 || v.mpeSemis < 0.0)   key += v.mpeSemis;
    if (v.neSemis > 0.0 || v.neSemis < 0.0)     key += v.neSemis;
    const double st0 = static_cast<double> (sp.keyTrack) * key + static_cast<double> (sp.pitchSt);
    return st0;
}

double Engine::voiceRate (const Voice& v, const SlotTape& tape, const GiGridParams::Stream& sp, std::int64_t t) const noexcept
{
    // st clamped to ±48; rate = 2^(st/12) · tapeRate / fs (double).
    const double st0 = voiceSemis (v, sp, t);
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
    return voiceRate (v, *tp, s, absPos);
}

double Engine::uncappedPeriod (const GiGridParams::Stream& sp) const noexcept
{
    return thinnedPeriod (fs, static_cast<double> (sp.density), lMaxSamples (sp), kQuotaUncapped);
}

double Engine::lMaxSamples (const GiGridParams::Stream& sp) const noexcept
{
    const double ms = static_cast<double> (sp.sizeMs) * (1.0 + 0.5 * static_cast<double> (sp.sizeRand));
    return std::clamp (ms, kMinSizeMs, kMaxSizeMs) * 0.001 * fs;
}

double Engine::lenBarSamples (const GiGridParams::Stream& sp) const noexcept
{
    return std::clamp (static_cast<double> (sp.sizeMs), kMinSizeMs, kMaxSizeMs) * 0.001 * fs;
}

double Engine::jitterOf (const GiGridParams::Stream& sp) const noexcept
{
    double J = static_cast<double> (sp.jitter);
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
    const auto& sp = pairSp (k, s);
    const SlotTape* tp = current[(size_t) s];
    const Voice& v = voices[k];
    if (tp == nullptr || ! (tp->rate > 0.0) || v.isFree())
        return 0.0;

    // Span in OUTPUT samples (the parent's spanSamples): W tape frames / tapeRate · fs.
    const double span = pairRegion (k, s).width() * fs / tp->rate;
    const PitchSel& sel = pairSel (k, s);
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
        const double r     = voiceRate (v, *tp, sp, nowT);
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
    const auto& sp = pairSp (k, s);
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

/** PLAN-2.4 PD8: the feedback tap gain of pair (k, s), g_fb = 1 / (S1 + 1)
    (parent fbTapGain / fbTapS1, OutputLaw.h:199-212), from the same nEff as
    the pair's output law. Hook fbArchLaw: the ARCH §10 law (FL8 control). */
float Engine::pairFbGain (int k, int s, double nEff) const noexcept
{
    const auto& sp = pairSp (k, s);
    WindowSel win;
    win.step = WindowMorphBank::stepFor (sp.window);
    win.tilt = ouaricon::grain::WindowLut::makeTilt (0.5f + 0.45f * sp.windowSkew);
    const double s1 = fbTapS1 (nEff, nEff, win, morphBank);
   #if OGI_TEST_HOOKS
    if (hooks.fbArchLaw)
        return fbTapGainArch (s1);
   #endif
    return fbTapGain (s1);
}

/** fb HP / LP coefficients from the ramped log-cutoffs (parent
    Engine.cpp:240-251 minus the matrix): tan only when a cutoff moved. */
void Engine::updateChainCutoffs() noexcept
{
    float lo = std::exp2 (fbLowLogRamp.current()), hi = std::exp2 (fbHighLogRamp.current());
    // PLAN-2.5a PD6 (parent :240-251): the routed cutoffs from the ramped raw Hz.
    if (matrix.routed (GiDest::FbLowCut))  lo = matrix.apply (GiDest::FbLowCut,  lo, matrix.offset (GiDest::FbLowCut));
    if (matrix.routed (GiDest::FbHighCut)) hi = matrix.apply (GiDest::FbHighCut, hi, matrix.offset (GiDest::FbHighCut));
    traceAt (GiDest::FbLowCut, lo);
    traceAt (GiDest::FbHighCut, hi);
    if (juce::exactlyEqual (lo, chainLowHz) && juce::exactlyEqual (hi, chainHighHz))
        return;
    chainLowHz = lo;
    chainHighHz = hi;
    for (auto& c : chain)
        c.setCutoffs (static_cast<double> (lo), static_cast<double> (hi));
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
    // PLAN-2.5b PD12: per-voice pitch of each counted voice — glide at the
    // tick + member bend + NE + (note − the note its serial started on);
    // +0.0 in bypass, so the key compares as in 2.5a. Hook pvKeyOff (GL6n).
    bool pvOff = false;
   #if OGI_TEST_HOOKS
    pvOff = hooks.pvKeyOff;
   #endif
    for (int k = 0; k < kVoiceSlots; ++k)
    {
        const Voice& v = voices[k];
        double pv = 0.0;
        if (! pvOff && v.isCounted())
            pv = v.glideAt (nowT) + v.mpeSemis + v.neSemis + static_cast<double> (v.note - v.serialNote);
        key[i++] = pv;
    }
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
        hooks.voiceRate[k]  = voiceRate (v, *current[0], 0, nowT);
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

#if OGI_TEST_HOOKS
float Engine::getLawFreshForTest (int k, int s, std::int64_t t) noexcept
{
    k = std::clamp (k, 0, kVoiceSlots - 1);
    s = std::clamp (s, 0, kGiStreams - 1);
    const std::int64_t keep = nowT;
    const double keepC = coher[k][s];
    nowT = t;
    const bool pr = pairRoutedK[k][s];
    const double nEff = effectiveCount (pr ? lenBarSamples (view[k][s]) : lenBarSamples (s), periodT[k][s]);
    const float g = pairLaw (k, s, nEff);
    nowT = keep;
    coher[k][s] = keepC;
    return g;
}
#endif

//==============================================================================
void Engine::process (float* outL, float* outR, int numSamples, const juce::MidiBuffer& midi,
                      std::uint64_t seedIn, const TransportSnapshot& transport,
                      const GiNeEvent* ne, int numNe, const float* scL, const float* scR) noexcept
{
    // Nothing to zero on this path (no samples, or no output pointer); the
    // processor clears the whole buffer whenever it does not call process().
    if (numSamples <= 0 || outL == nullptr || outR == nullptr)
        return;

    // PLAN-2.6 PD6 / PD19: only the negotiated sidechain channels are read
    // (ch 1 of a mono sidechain is host garbage, P14).
    scChNow = scL == nullptr ? 0 : (scR == nullptr ? 1 : 2);
    float scPk = 0.0f;
    bool scZero = true;
   #if OGI_TEST_HOOKS
    scZero = ! hooks.scNoClear;
   #endif

    // PLAN-2.5a PD12: the host transport, once per host block (R39 held anchor).
    tracker.update (transport, absPos);

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

    // PLAN-2.6 PD5: rec_arm / target / trigger read once; the arm edge is
    // stamped at the block start (a host changes parameters between blocks).
    recorder.beginBlock (blockStart, scChNow, tracker);

    // PLAN-2.5b PD6: the stamped VST3 raw events. Offsets are clamped into the
    // block and made non-decreasing (host order kept); NoteOns go to the
    // per-block correlation list up front (a MIDI note-on takes its noteId).
    numNe = (ne != nullptr) ? std::clamp (numNe, 0, kNeCapacity) : 0;
    int neOff[kNeCapacity];
    numNeOn = 0;
    neBlockStart = blockStart;
    bool moduleTable = false;
   #if OGI_TEST_HOOKS
    moduleTable = hooks.neModuleTable;
    if (moduleTable)
        std::fill (std::begin (neTable), std::end (neTable), 0.0);
   #endif
    {
        int last = 0;
        for (int i = 0; i < numNe; ++i)
        {
            last = std::max (last, std::clamp (ne[i].offset, 0, numSamples - 1));
            neOff[i] = last;
            if (ne[i].type == 0 && numNeOn < kNeCapacity)
                neOn[numNeOn++] = { last, ne[i].channel, ne[i].pitch, ne[i].noteId, false };
        }
        // neModuleTable (NE2n / NE3 control): the module's model — a tuning
        // reaches a pitch-keyed table only when its noteId's NoteOn is in this
        // block, and is read once at note-on (NoteExpression_VST3.cpp:216-235).
        if (moduleTable)
            for (int i = 0; i < numNe; ++i)
                if (ne[i].type == 2 && ne[i].typeId == kTuningTypeID)
                    for (int j = 0; j < numNeOn; ++j)
                        if (neOn[j].noteId == ne[i].noteId && neOn[j].pitch >= 0 && neOn[j].pitch < 128)
                            neTable[neOn[j].pitch] = 240.0 * (ne[i].value - 0.5);
    }
    int ni = 0;

    int pos = 0;
    while (pos < numSamples)
    {
        const std::int64_t t = blockStart + pos;
        const int cellOff = static_cast<int> (t & static_cast<std::int64_t> (kCell - 1));
        if (cellOff == 0)
            gridTick (t);

        const int len = std::min (numSamples - pos, kCell - cellOff);
        const int b = pos + len;

        // PLAN-2.6 PD19: the sidechain of [pos, b) into the scratch FIRST (the
        // buffer is in place: outL / scL alias), then the output zeroed. The
        // zero keeps the leak guarantee local even if the mixer ever
        // accumulates; mixPiece assigns every sample, so renders are unchanged.
        for (int i = 0; i < len; ++i)
        {
            scS[0][cellOff + i] = scL != nullptr ? scL[pos + i] : 0.0f;
            scS[1][cellOff + i] = scR != nullptr ? scR[pos + i] : 0.0f;
            scPk = std::max ({ scPk, std::abs (scS[0][cellOff + i]), std::abs (scS[1][cellOff + i]) });
        }
        if (scZero)
        {
            std::fill (outL + pos, outL + b, 0.0f);
            std::fill (outR + pos, outR + b, 0.0f);
        }

        int seg = pos;
        for (;;)
        {
            // Merged cursor: MIDI first at a tie (the NoteOn exists before its NE).
            const int mPos = cursor.nextPos();
            const int nPos = ni < numNe ? neOff[ni] : INT_MAX;
            const int nx = std::min (mPos, nPos);
            if (nx >= b)
                break;
            const bool isMidi = mPos <= nPos;
            const int at = std::max (isMidi ? mPos : nPos, seg);
            if (at > seg)
            {
                runSegment (cellOff + (seg - pos), cellOff + (at - pos), blockStart + seg);
                seg = at;
            }
            if (isMidi)
            {
                const MidiEvent e = cursor.pop();
                applyEvent (e, blockStart + at);
            }
            else
            {
                nowT = blockStart + at;
                if (! moduleTable)
                    applyNe (ne[ni]);
                ++ni;
            }
        }
        if (b > seg)
            runSegment (cellOff + (seg - pos), cellOff + (b - pos), blockStart + seg);

        // PLAN-2.6 PD19: the capture runs over this piece's scratch after the
        // event loop (Next Note stamps arrive from startVoice / retriggerVoice).
        recorder.runPiece (cellOff, cellOff + len, t, scS, tracker);

        mixPiece (cellOff, cellOff + len, outL + pos, outR + pos, t);
        pos = b;
    }
    absPos = blockStart + numSamples;
    publishTelemetry (outL, outR, numSamples, scPk);
    serviceTapeSnapshot();   // PLAN-3.2 PD18: after the block's last tape write; reads only
}

//==============================================================================
// PLAN-2.6 T11 / PD15: the Stage 3 feeds. Written here only; never read back.
void Engine::publishTelemetry (const float* outL, const float* outR, int n, float scPk) noexcept
{
    float op = 0.0f;
    for (int i = 0; i < n; ++i)
        op = std::max ({ op, std::abs (outL[i]), std::abs (outR[i]) });
    pushMax (tel.outPeak, op);
    pushMax (tel.scInPeak, scPk);

    for (int s = 0; s < kTapeSlots; ++s)
    {
        tel.headPos[(size_t) s].store (head[s].h, std::memory_order_relaxed);
        const std::uint8_t sw = static_cast<std::uint8_t> ((handoff.pendingDepth (s) > 0 ? 1 : 0)
                                                          | (xfade[s].active ? 2 : 0)
                                                          | (head[s].parked ? 4 : 0)
                                                          | (head[s].swapWaiting ? 8 : 0));
        tel.swapState[(size_t) s].store (sw, std::memory_order_relaxed);
    }

    // One complete frame per host block (TripleBuffer: seqHead first, seqTail last).
    auto& f = tel.frames.writeBuffer();
    const std::uint64_t sq = ++tel.seq;
    f.seqHead = sq;
    int count = 0;
    const VoiceGrain* const base = pool.getGrains().data();
    for (int i = 0; i < GiPool::kNumSlots && count < GiUiFrame::kMaxGrains; ++i)
    {
        if (! pool.isActive (i))
            continue;
        const VoiceGrain& vg = base[i];
        const double frames = vg.tape != nullptr ? static_cast<double> (vg.tape->frames) : 0.0;
        auto& u = f.grains[count++];
        u.voice   = static_cast<std::uint8_t> (vg.voice);
        u.stream  = static_cast<std::uint8_t> (vg.slot);
        u.flags   = static_cast<std::uint8_t> ((vg.g.dir < 0 ? 1 : 0) | ((i >= kPoolLive || spawner.isKilling (vg, i)) ? 2 : 0));
        u.normPos = frames > 0.0 ? static_cast<float> (vg.g.readAbs / frames) : 0.0f;
        u.gain    = vg.g.killGain;
        // PLAN-3.2 PD1: field copies for the lane painter (reader-side derivation).
        u.windowStep = static_cast<std::uint8_t> (std::clamp (vg.g.windowStep, 0, 255));
        u.gainL      = vg.g.gainL;
        u.gainR      = vg.g.gainR;
        u.phase      = static_cast<float> ((static_cast<double> (vg.g.age) + vg.g.frac) * vg.g.invLen);
        u.lenSamples = static_cast<float> (vg.g.len);
        u.tiltT      = vg.g.tiltT;
        u.tiltA      = vg.g.tiltA;
        u.tiltB      = vg.g.tiltB;
    }
    f.count = count;
    for (int k = 0; k < kVoiceSlots; ++k)
    {
        const Voice& v = voices[k];
        auto& u = f.voices[k];
        u.note = v.isCounted() ? static_cast<std::int8_t> (std::clamp (v.note, 0, 127)) : static_cast<std::int8_t> (-1);
        u.amp  = static_cast<float> (v.env.getLevel());
        for (int s = 0; s < kGiStreams; ++s)
        {
            const SlotTape* tp = current[(size_t) s];
            if (tp == nullptr || ! v.isCounted() || ! (tp->frames > 0))
            {
                u.scan[s] = 0.0f;
                continue;
            }
            const auto& sp = pairSp (k, s);
            const auto& rg = pairRegion (k, s);
            const double sigmaSec = (sp.scanMode == kScanFreeRun) ? freeRun[s].at (absPos) : v.scan[s].at (absPos);
            const double c = rg.width() > 0.0 ? scanCentre (rg, static_cast<double> (sp.position), sigmaSec * tp->rate, sp.scanMode) : 0.0;
            u.scan[s] = static_cast<float> (c / static_cast<double> (tp->frames));
        }
    }
    f.seqTail = sq;
    tel.frames.publish();
}

// PLAN-3.2 PD18 (GiTapeSnapshot.h): the Save-recording copy-out. Audio thread,
// once per process() / bypassedBlock() call: one relaxed load while nothing is
// in flight; else at most kChunkFrames level-0 frames of the slot's tape into
// the requester's buffers. The tape is only read; no allocation, no lock.
void Engine::serviceTapeSnapshot() noexcept
{
    using S = GiTapeSnapshot;
    if (! tapeSnap.audioActive())
        return;

    const S::State st = tapeSnap.state();   // acquire: the request fields are visible from Requested on
    if (st == S::CancelRequested)
    {
        tapeSnap.audioAckCancel();
        return;
    }
    if (st != S::Requested && st != S::Copying)
        return;

    const int s = tapeSnap.slot;
    const SlotTape* const tp = (s >= 0 && s < kTapeSlots) ? current[(size_t) s] : nullptr;
    const std::uint32_t epoch = (s >= 0 && s < kTapeSlots) ? tel.overview.epoch (s) : 0u;

    if (st == S::Requested)
    {
        if (tp == nullptr || tapeSnap.L == nullptr || tapeSnap.R == nullptr || tapeSnap.frames <= 0 || tp->frames != tapeSnap.frames)
        {
            tapeSnap.audioMove (S::Requested, S::Aborted);
            return;
        }
        tapeSnap.tape      = tp;
        tapeSnap.tapeEpoch = epoch;
        tapeSnap.cursor    = 0;
        if (! tapeSnap.audioBegin())
            return;   // cancelled before the first chunk (acknowledged)
    }
    else if (tp == nullptr || tp != tapeSnap.tape || tp->frames != tapeSnap.frames || epoch != tapeSnap.tapeEpoch)
    {
        // The slot's tape changed (take, clear, Reset): `changed`. The epoch
        // is bumped at every take, so a new tape at a recycled address counts.
        tapeSnap.audioMove (S::Copying, S::Aborted);
        return;
    }

    const int n = std::min (S::kChunkFrames, tapeSnap.frames - tapeSnap.cursor);
    float* const dl = tapeSnap.L + tapeSnap.cursor;
    float* const dr = tapeSnap.R + tapeSnap.cursor;
    for (int i = 0; i < n; ++i)
    {
        const std::int64_t fr = static_cast<std::int64_t> (tapeSnap.cursor) + i;
        dl[i] = tp->tape.levelSample (0, 0, fr);
        dr[i] = tp->tape.levelSample (0, 1, fr);
    }
    tapeSnap.cursor += n;
    tapeSnap.audioBumpProgress();
    if (tapeSnap.cursor >= tapeSnap.frames)
        tapeSnap.audioMove (S::Copying, S::Done);
}

void Engine::foldOverview (int s) noexcept
{
    // The cell the head just wrote: 32 frames, possibly across a wrap. Folded
    // into the running bin accumulator; the bin is stored at every cell.
    const SlotTape* tp = current[(size_t) s];
    if (tp == nullptr)
        return;
    tel.ovWrites[(size_t) s].fetch_add (1, std::memory_order_relaxed);   // PLAN-3.2 PD6 (never read here)
    auto& a = tel.acc[s];
    for (int o = 0; o < kCell; ++o)
    {
        const std::int64_t fr = head[s].headAt (o);
        const int b = static_cast<int> (fr / kOverviewBin);
        const float l = tp->tape.levelSample (0, 0, fr), r = tp->tape.levelSample (0, 1, fr);
        if (b != a.bin)
        {
            if (a.bin >= 0)
                tel.overview.write (s, a.bin, a.lo, a.hi);
            a.bin = b;
            a.lo = std::min (l, r);
            a.hi = std::max (l, r);
        }
        else
        {
            a.lo = std::min ({ a.lo, l, r });
            a.hi = std::max ({ a.hi, l, r });
        }
    }
    if (a.bin >= 0)
        tel.overview.write (s, a.bin, a.lo, a.hi);
}

//==============================================================================
// Grid tick
void Engine::gridTick (std::int64_t t) noexcept
{
    nowT = t;
    gp.latch (ptrs);
    recorder.gridTick();   // PLAN-2.6 PD5: threshold + gain (20 ms ramp) latch
    modeSwitch();   // PLAN-2.5b T7 (D32): engine events before the sample's host events

    // PLAN-2.5a PD5 step 2: the modulation sources of this cell (sampled once).
    tickSources (t);
    matrixTick (t);

    // PLAN-2.5a PD12: Tempo mode reads the EFFECTIVE density / size (quotas,
    // coherence, the spawn size and the clock all see the synced values, R45).
    const bool tempoNow = gp.syncMode == 1;
    if (tempoNow)
        applyTempoEffective();
    if (tempoNow != prevTempo)
    {
        // Free ↔ Tempo folds the onset count into the epoch (P14 pattern): the
        // draws never replay.
        for (int k = 0; k < kVoiceSlots; ++k)
        {
            Voice& v = voices[k];
            if (! v.isCounted())
                continue;
            for (int s = 0; s < kGiStreams; ++s)
            {
                v.epoch[s] += prevTempo ? tclk[k][s].getOnsetCount() : v.sched[s].getOnsetCount();
                v.sched[s].reset();
                tclk[k][s].restart();
                burstPending[k][s] = false;
            }
        }
        prevTempo = tempoNow;
    }
   #if OGI_TEST_HOOKS
    hooks.relocks = static_cast<std::uint64_t> (tracker.getRelocks());
   #endif

    for (int s = 0; s < kGiStreams; ++s)
    {
        jit[s] = jitterOf (s);

        // P14: an enable edge while a voice sounds folds the onset count into
        // the epoch and restarts the scheduler at this tick (draws never replay).
        const bool en = gp.s[s].enable;
        if (en && ! prevEnable[s])
        {
            for (int k = 0; k < kVoiceSlots; ++k)
            {
                Voice& v = voices[k];
                if (! v.isCounted())
                    continue;
                v.epoch[s] += tempoNow ? tclk[k][s].getOnsetCount() : v.sched[s].getOnsetCount();
                v.sched[s].reset();
                tclk[k][s].restart();
            }
        }
        prevEnable[s] = en;
    }

    outGainRamp.setTarget (dbToGain (gp.outputGainDb), rampCells20);
    outGainRamp.tick();
    setStreamRamps (false);

    // PLAN-2.5a PD6 (parent R47, 82406fce Engine.cpp:698-755): routed ramped
    // G destinations are modulated AFTER the base ramp, in that ramp's domain.
    {
        using namespace GiDest;
        const bool ro = matrix.routed (OutputGain);
        const float go = ro ? dbToGain (matrix.apply (OutputGain, GiModMap::gainToRaw (outGainRamp.current()), matrix.offset (OutputGain)))
                            : outGainRamp.current();
        outGainMod.setCell (outGainNow, go);
        outGainNow = go;
        useOutMod = ro || matrix.routedPrev (OutputGain);
        traceAt (OutputGain, go);
    }
    applyLevelPanMod();
    updatePairMix();   // PLAN-2.5a PD7

    // PLAN-2.4 PD11 / P9: reverb atomics at the tick only (block-size invariant).
    writeReverbAtomics();

    // PLAN-2.4 PD7: chain controls (20 ms), cutoffs as log2 ramps of raw Hz
    // (parent updateChainCutoffs: tan only when a cutoff moved).
    parityKnob.setTarget (kFbKnob * gp.feedback, rampCells20);
    parityKnob.tick();
    diffRamp.setTarget (gp.fbDiffusion, rampCells20);
    diffRamp.tick();
    satDRamp.setTarget (1.0f + kSatSpan * gp.fbSaturation, rampCells20);
    satDRamp.tick();
    {
        // PLAN-2.5a PD6 (parent :772-785): diffusion / saturation substitutes.
        using namespace GiDest;
        const bool rd = matrix.routed (FbDiff), rs = matrix.routed (FbSat);
        const float df = rd ? GiModMap::pctFromRaw (matrix.apply (FbDiff, diffRamp.current() * 100.0f, matrix.offset (FbDiff)))
                            : diffRamp.current();
        const float sd = rs ? GiModMap::satDFromRaw (matrix.apply (FbSat, GiModMap::satDToRaw (satDRamp.current()), matrix.offset (FbSat)))
                            : satDRamp.current();
        diffMod.setCell (diffNow, df); diffNow = df;
        satDMod.setCell (satDNow, sd); satDNow = sd;
        useDiffMod = rd || matrix.routedPrev (FbDiff);
        useSatMod  = rs || matrix.routedPrev (FbSat);
        traceAt (FbDiff, df);
        traceAt (FbSat, sd);
    }
    fbLowLogRamp.setTarget (std::log2 (gp.fbLowCutHz), rampCells20);
    fbLowLogRamp.tick();
    fbHighLogRamp.setTarget (std::log2 (gp.fbHighCutHz), rampCells20);
    fbHighLogRamp.tick();
    updateChainCutoffs();
   #if OGI_TEST_HOOKS
    {
        const bool on = gp.reverbMix > 0.0f;
        if (hooks.reverbHardCut && revMixWasOn && ! on)
            reverb.reset();   // RV2 control: the tail is cut, no 50 ms fade
        revMixWasOn = on;
    }
   #endif

    updateHeads (t);   // PLAN-2.4 PD4: λ / run ramps, freeze, park (the swap rule reads it)
    takeTapes (t);
    finishXfades (t);
    retireScan();
    updateRegion();
    for (int s = 0; s < kTapeSlots; ++s)
        if (headRelatch[s] && current[(size_t) s] != nullptr)
        {
            head[s].relatch (region[s].lo, region[s].hi, current[(size_t) s]->frames);   // PD5: new tape → region lo, parked
            headRelatch[s] = false;
        }
    enforcePolyphony();
    updateScale();
    updateQuotas();
    updateScan (t);
    writeHeads (t);    // PLAN-2.4 PD3: the whole cell, before any MIDI or segment of it

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
        if (! head[s].parked && handoff.pendingDepth (s) > 0)
        {
            // PLAN-2.4 PD5: every take waits for a parked head (the run target
            // went false in updateHeads; ≤ 20 ms + 1 cell).
           #if OGI_TEST_HOOKS
            ++hooks.swapWaits;
           #endif
            continue;
        }

        SlotTape* p = handoff.take (s);
        if (p == nullptr)
            continue;

       #if OGI_TEST_HOOKS
        ++hooks.tapesTaken;
       #endif
        SlotTape* old = current[(size_t) s];
        if (SlotHandoff::isClear (p))
        {
            // PLAN-2.6 PD11: the clear sentinel — the slot's grains get the
            // 5 ms kill fade, the old tape retires, the slot is empty. The
            // sentinel itself is never held or retired.
            current[(size_t) s] = nullptr;
            headRelatch[s] = false;
            tel.overview.bumpEpoch (s);
            tel.acc[s].bin = -1;
            if (old != nullptr)
            {
                killGrainsOfTape (old);
                retiring[numRetiring++] = old;
            }
            continue;
        }
        current[(size_t) s] = p;
        // PLAN-2.4 PD5: a Reset take (same source, same rate) keeps the head;
        // any other take (new file, SR rebuild, first load) re-latches it.
        const bool resetTake = old != nullptr && p->rebuild && old->generation == p->generation
                            && kaiser::ratesEqual (old->rate, p->rate);
        if (! resetTake)
            headRelatch[s] = true;
        // PLAN-2.6 T11: any take rebuilds the tape from pristine (a Reset, an
        // SR rebuild, a new source) — the head-written bins are stale.
        tel.overview.bumpEpoch (s);
        tel.acc[s].bin = -1;
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
            if (hooks.resetHardSwap && resetTake)
                x.len = 1;   // CK4 control: the grains switch tape at the take
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
   #if OGI_TEST_HOOKS
    spawner.testKillStepOverride = hooks.swapKillHard ? 1.0f : 0.0f;
   #endif
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
        {
            // PD6: a routed pair demands from its own view.
            const double nk = pairRoutedK[k][s] ? streamDemand (static_cast<double> (view[k][s].density),
                                                                 static_cast<double> (view[k][s].sizeMs), fs)
                                                : n[s];
            demand[k * kGiStreams + s] = (canSpawn[s] && voices[k].isCounted()) ? nk : 0.0;
        }

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
                      || ! sameBits (lawKey, prevLawKey, sizeof (lawKey))
                      || numPairsRouted > 0 || numPairsRoutedPrev > 0;   // PD5 / P8: routed pairs recompute every tick
    if (! changed)
    {
        // Inputs unchanged: hold every law gain (a CellLerp needs a push per tick).
        for (int k = 0; k < kVoiceSlots; ++k)
            for (int s = 0; s < kGiStreams; ++s)
            {
                lawGain[k][s].push (lawTarget[k][s]);
                gFbLerp[k][s].push (gFbTarget[k][s]);
            }
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
   #if OGI_TEST_HOOKS
    ++hooks.quotaRecomputes;
    ++hooks.lawRecomputes;
   #endif

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
            const bool pr = pairRoutedK[k][s];
            const double J = pr ? jitKS[k][s] : jit[s];
            double T = pr ? thinnedPeriod (fs, static_cast<double> (view[k][s].density), lMaxSamples (view[k][s]),
                                           periodOff ? kQuotaUncapped : m, J)
                          : thinnedPeriod (fs, density[s], lMax[s], periodOff ? kQuotaUncapped : m, J);
            // R§1.3 / R§8: a quota DECREASE re-times the pending gap at once with
            // the jittered gap of the pending onset (a pure function of its index
            // and the grid snapshot: block-size invariant); an increase lets the
            // latched gap roll.
            if (gp.syncMode == 1)
            {
                // PD12 (corrects ARCH §9): Tempo thinning on the DIVISION —
                // div = tempoDiv·2^j, j = min { j >= 0 : I·2^j >= T }, so a
                // thinned stream drops only power-of-two onsets and stays on
                // the bar-anchored grid. Hook tempoContinuousThin: the
                // continuous period (TS2n control, off-grid).
                const TempoAnchor& A = tracker.anchor();
                const double div0 = pr ? tempoDivKS[k][s] : tempoDivS[s];
                bool cont = false;
               #if OGI_TEST_HOOKS
                cont = hooks.tempoContinuousThin;
               #endif
                if (cont)
                    divKS[k][s] = T * A.pps;
                else
                {
                    const double I = div0 / A.pps;
                    double Tt = I;
                    int j = 0;
                    while (Tt < T * (1.0 - 1.0e-6) && j < 40)   // 1e-6: the float density's rounding (I == fs / density up to ~6e-8)
                    {
                        Tt *= 2.0;
                        ++j;
                    }
                    divKS[k][s] = std::ldexp (div0, j);
                    T = Tt;
                }
            }
            else if (v.isCounted() && T > periodT[k][s])
            {
                const std::uint64_t cnt = v.sched[s].getOnsetCount();
                const double gap = cnt > 0 ? jitterGap (T, J, s, v.serial, v.epoch[s], cnt - 1) : T;
                v.sched[s].rescheduleNext (gap);
            }
            periodT[k][s] = T;
            const double nEff = effectiveCount (pr ? lenBarSamples (view[k][s]) : lenBar[s], T);
            lawTarget[k][s] = pairLaw (k, s, nEff);
            lawGain[k][s].push (lawTarget[k][s]);
            gFbTarget[k][s] = pairFbGain (k, s, nEff);   // PLAN-2.4 PD8
            gFbLerp[k][s].push (gFbTarget[k][s]);
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
        for (int k = 0; k < kVoiceSlots; ++k)
        {
            auto& v = voices[k];
            if (v.isFree())
                continue;
            if (pairRoutedK[k][s])   // PD6: the voice's own scan rate / read region
                v.scan[s].reanchor (t, static_cast<double> (view[k][s].scanRate) / fs,
                                    scanPeriodSec (pairRegion (k, s), tapeRate, sp.scanMode));
            else
                v.scan[s].reanchor (t, rateSec, voicePeriod);
        }
    }
}

//==============================================================================
// 2.5a sources (PLAN-2.5a T5, PD10 / PD11)
ModEnvParams Engine::menvParams (int e) const noexcept
{
    const auto& g = gp.menv[std::clamp (e, 0, kGiModEnvs - 1)];
    ModEnvParams p;
    p.attackMs  = g.attackMs;
    p.decayMs   = g.decayMs;
    p.sustain   = g.sustain;
    p.releaseMs = g.releaseMs;
    return p;
}

/** PD9: per-voice LFO N ticks while its mode is Per-voice or the 20 ms
    crossfade has not returned to exactly 0. */
bool Engine::lfoPerVoiceActive (int n) const noexcept
{
    return gp.lfoMode[n] != 0 || ! lfoXf[n].settledAt (0.0f);
}

/** PD10: the per-voice S&H / smooth-random seed, keyed on the note serial
    (hook slotKeyedSeed: on the voice SLOT — the VL2 control). */
std::uint64_t Engine::voiceLfoSeed (std::uint64_t key) const noexcept
{
    return mix64 (seed ^ kVoiceLfoStream ^ mix64 (key));
}

/** Note-on: the voice's sources (velocity, key, note random, pressure /
    slide / bend from channel state, Mod Envs from 0, per-voice LFOs prepared
    at lfoN_phase). */
void Engine::initVoiceMod (int k) noexcept
{
    auto& m = vm[k];
    const Voice& v = voices[k];
    bool noReset = false, slotKey = false;
   #if OGI_TEST_HOOKS
    noReset = hooks.pvLfoNoReset;
    slotKey = hooks.slotKeyedSeed;
   #endif
    // VL1 control (pvLfoNoReset): the new voice adopts a running voice's LFO
    // state instead of restarting at lfoN_phase.
    int donor = -1;
    if (noReset)
        for (int j = 0; j < kVoiceSlots && donor < 0; ++j)
            if (j != k && ! voices[j].isFree() && vm[j].lfoOn[0])
                donor = j;

    m.clear();
    m.pressure = chanPressure;
    m.slide    = chanSlide;
    m.bendSrc  = bend;
    if (gp.mpeEnable)
    {
        // PLAN-2.5b PD8: under MPE the per-voice sources come from the note's member channel.
        const int ch = v.channel;
        const bool mem = mpe.isMember (ch);
        m.pressure = mem ? mpe.pressure[ch] : 0.0f;
        m.slide    = mem ? mpe.slide[ch] : 0.0f;
        m.bendSrc  = mem ? mpe.bend[ch] : 0.0f;
    }
    for (int e = 0; e < kGiModEnvs; ++e)
        m.env[e].noteOn (menvParams (e), fs);

    const bool tempoMode = gp.syncMode == 1;
    const TempoAnchor& A = tracker.anchor();
    for (int n = 0; n < kGiLfos; ++n)
    {
        m.lfoRateOff[n] = 0.0f;
        m.lfoRateHz[n] = tempoMode ? fs * A.pps / tempoDivBeats (gp.lfo[n].rateSync, A.barLen)
                                   : static_cast<double> (gp.lfo[n].rateHz);
        if (! lfoPerVoiceActive (n))
            continue;
        if (donor >= 0 && vm[donor].lfoOn[n])
            m.lfo[n] = vm[donor].lfo[n];
        else
            m.lfo[n].prepare (fs, rampCellsMod, gp.lfo[n].phaseDeg);
        m.lfoOn[n] = true;
    }

    // Source values of the voice's first cell (the LFOs: the value their first
    // tick will produce — frac 0, phase lfoN_phase).
    const std::uint64_t key = slotKey ? static_cast<std::uint64_t> (k) : v.serial;
    for (int i = 0; i < kGiModSources; ++i)
        m.src[i] = srcG[i];
    for (int n = 0; n < kGiLfos; ++n)
    {
        if (! m.lfoOn[n])
            continue;
        double ph = static_cast<double> (m.lfo[n].getPhaseDeg()) / 360.0 + m.lfo[n].getFrac();
        ph -= std::floor (ph);
        const float pv = Lfo::shape (gp.lfo[n].shape, ph, m.lfo[n].getCycle(), voiceLfoSeed (key), n);
        const float xf = lfoXf[n].current();
        m.src[GiSrc::Lfo1 + n] = (1.0f - xf) * srcG[GiSrc::Lfo1 + n] + xf * pv;
    }
    m.src[GiSrc::ModEnv1]    = 0.0f;
    m.src[GiSrc::ModEnv2]    = 0.0f;
    m.src[GiSrc::Velocity]   = v.velocity;
    m.src[GiSrc::Key]        = std::clamp (static_cast<float> (v.note - 60) / 64.0f, -1.0f, 1.0f);
    m.src[GiSrc::NoteRandom] = ouaricon::grain::CounterRng::Draw (seed, kNoteRandStream, key).nextFloat (0);
    m.src[GiSrc::Pressure]   = m.pressure;
    m.src[GiSrc::Slide]      = m.slide;
    m.src[GiSrc::Bend]       = m.bendSrc;

    // PD4: the voice's offsets / views at note-on (the slot depths of now).
    voicePass (k, true);
    buildPairViews (k);
    if (gp.syncMode == 1)
        pairTempo (k);
    voiceLfoRates (k);
    for (int s = 0; s < kGiStreams; ++s)
        if (pairMix[s])
        {
            float gl = 1.0f, gr = 1.0f, lv = 1.0f;
            pairGainTarget (k, s, gl, gr, lv);
            pairGL[k][s].reset (gl);
            pairGR[k][s].reset (gr);
            pairLv[k][s].reset (lv);
        }
}

/** The voice's own LFOs for this cell (PD9 / PD12: Free = the modulated rate
    of the previous cell; Tempo = a tempo-rate free run from note-on, P5). */
void Engine::tickVoiceLfos (int k, std::int64_t t) noexcept
{
    auto& m = vm[k];
    bool slotKey = false;
   #if OGI_TEST_HOOKS
    slotKey = hooks.slotKeyedSeed;
   #endif
    const std::uint64_t sd = voiceLfoSeed (slotKey ? static_cast<std::uint64_t> (k) : voices[k].serial);
    for (int n = 0; n < kGiLfos; ++n)
    {
        const float g = srcG[GiSrc::Lfo1 + n];
        if (! lfoPerVoiceActive (n))
        {
            m.lfoOn[n] = false;
            m.src[GiSrc::Lfo1 + n] = g;
            continue;
        }
        if (! m.lfoOn[n])
        {
            // Global → Per-voice (PD9): re-prepared at lfoN_phase at the switch tick.
            m.lfo[n].prepare (fs, rampCellsMod, gp.lfo[n].phaseDeg);
            m.lfoOn[n] = true;
        }
        const float pv = m.lfo[n].tick (n, gp.lfo[n], false, tracker, t, m.lfoRateOff[n], m.lfoRateHz[n], sd, 0);
        const float xf = lfoXf[n].current();
        m.src[GiSrc::Lfo1 + n] = (1.0f - xf) * g + xf * pv;
    }
}

/** PD5 step 2: global LFOs (the VERBATIM class, parent tickSources), macros
    (20 ms), the wheel, the lfoN_mode crossfades, then per voice the LFOs,
    Mod Env levels and MIDI sources. Rates come from the PREVIOUS cell (R50.5). */
void Engine::tickSources (std::int64_t t) noexcept
{
    const bool tempoMode = gp.syncMode == 1;
    for (int n = 0; n < kGiLfos; ++n)
        srcG[GiSrc::Lfo1 + n] = lfos[n].tick (n, gp.lfo[n], tempoMode, tracker, t, lfoRateOff[n], lfoRateHz[n], seed, 0);

    bool macroSnap = false, xfSnap = false;
   #if OGI_TEST_HOOKS
    macroSnap = hooks.macroRampZero;
    xfSnap    = hooks.modeSwitchSnap;
   #endif
    for (int n = 0; n < kGiMacros; ++n)
    {
        if (macroSnap)
            macroRamp[n].reset (gp.macro[n]);   // PMc1 control: no 20 ms ramp
        else
        {
            macroRamp[n].setTarget (gp.macro[n], rampCellsMod);
            macroRamp[n].tick();
        }
        srcG[GiSrc::Macro1 + n] = macroRamp[n].current();
    }
    srcG[GiSrc::ModWheel] = static_cast<float> (wheelMsb * 128 + wheelLsb) / 16383.0f;

    for (int n = 0; n < kGiLfos; ++n)
    {
        const float x = gp.lfoMode[n] != 0 ? 1.0f : 0.0f;
        if (xfSnap)
            lfoXf[n].reset (x);                  // LM1n control: the mode switch steps
        else
        {
            lfoXf[n].setTarget (x, rampCellsMod);
            lfoXf[n].tick();
        }
    }

    for (int k = 0; k < kVoiceSlots; ++k)
    {
        if (voices[k].isFree())
            continue;
        auto& m = vm[k];
        tickVoiceLfos (k, t);
        m.src[GiSrc::ModEnv1]  = static_cast<float> (m.env[0].getLevel());
        m.src[GiSrc::ModEnv2]  = static_cast<float> (m.env[1].getLevel());
        m.src[GiSrc::ModWheel] = srcG[GiSrc::ModWheel];
        for (int n = 0; n < kGiMacros; ++n)
            m.src[GiSrc::Macro1 + n] = srcG[GiSrc::Macro1 + n];
        m.src[GiSrc::Pressure] = m.pressure;
        m.src[GiSrc::Slide]    = m.slide;
        m.src[GiSrc::Bend]     = m.bendSrc;
    }
    updateNewest();   // PD8: per-voice sources on the global path
}

/** PD8 (D4): newest = the highest serial among COUNTED voices (never a stolen
    spare), at the tick. A handover snapshots every mono value into `from` and
    restarts the shared 20 ms ramp (never stacked); no counted voice → held.
    LFO N on the global path (xf at exactly 0) is pinned to the global value. */
void Engine::updateNewest() noexcept
{
    int nk = -1;
    std::uint64_t best = 0;
    for (int k = 0; k < kVoiceSlots; ++k)
        if (voices[k].isCounted() && voices[k].serial > best)
        {
            best = voices[k].serial;
            nk = k;
        }
    bool snap = false, stack = false;
   #if OGI_TEST_HOOKS
    snap  = hooks.newestNoRamp;
    stack = hooks.newestStack;
   #endif
    if (nk >= 0 && best != newestSer)
    {
        const bool inRamp = ! newestW.settledAt (1.0f);
        for (int p = 1; p < kGiModSources; ++p)
        {
            // NV4 control (newestStack): a handover inside a ramp restarts from
            // the OLD voice's value instead of the current blend.
            float from = monoVal[p];
            if (stack && inRamp && newestSer != 0)
                for (int k = 0; k < kVoiceSlots; ++k)
                    if (! voices[k].isFree() && voices[k].serial == newestSer)
                        from = vm[k].src[p];
            monoFrom[p] = from;
        }
        if (snap)
            newestW.reset (1.0f);
        else
        {
            newestW.reset (0.0f);
            newestW.setTarget (1.0f, rampCellsMod);
        }
        newestSer = best;
       #if OGI_TEST_HOOKS
        ++hooks.handovers;
        if (voices[nk].isStolen())
            ++hooks.newestWasStolen;
       #endif
    }
    newestW.tick();
    const float w = newestW.current();
    for (int p = 1; p < kGiModSources; ++p)
    {
        const bool pv = GiSrc::isPerVoice (p) || (GiSrc::isLfo (p) && lfoPerVoiceActive (p - GiSrc::Lfo1));
        if (! pv)
        {
            monoFrom[p] = monoVal[p] = srcG[p];   // global value (pinned)
            continue;
        }
        if (nk >= 0)
            monoVal[p] = monoFrom[p] + (vm[nk].src[p] - monoFrom[p]) * w;
        else
            monoFrom[p] = monoVal[p];             // held when silent
        srcG[p] = monoVal[p];
    }
   #if OGI_TEST_HOOKS
    hooks.newestSerial = newestSer;
   #endif
}

/** PD16: relaxed atomics for the UI at the tick (never APVTS). */
void Engine::writeTelemetry() noexcept
{
    int nk = -1;
    for (int k = 0; k < kVoiceSlots; ++k)
        if (voices[k].isCounted() && voices[k].serial == newestSer)
            nk = k;
    matrix.writeTelemetry (destTel.data(), nk >= 0 ? vm[nk].offCur() : nullptr);
    for (int p = 1; p < kGiModSources; ++p)
        srcTel[(size_t) p].store (srcG[p], std::memory_order_relaxed);
    for (int n = 0; n < kGiLfos; ++n)
    {
        const double p = lfos[n].position() - static_cast<double> (lfos[n].getPhaseDeg()) / 360.0;
        float ph = static_cast<float> (p - std::floor (p));
        if (! (ph < 1.0f)) ph = 0.0f;
        lfoPhaseTel[(size_t) n].store (ph, std::memory_order_relaxed);
    }
}

//==============================================================================
// 2.5a matrix (PLAN-2.5a T6, PD3 / PD5 / PD6)
void Engine::traceAt (int dest, float v) noexcept
{
   #if OGI_TEST_HOOKS
    auto& tr = hooks.traceDest;
    if (tr.dest == dest && tr.buf != nullptr && tr.n < tr.len)
        tr.buf[tr.n++] = v;
   #else
    juce::ignoreUnused (dest, v);
   #endif
}

/** PD5 step 3: the global pass of this cell (the parent ModMatrix port), the
    LFO rates of the NEXT cell (R50.5), and the tick-destination overwrite of
    the latched gp fields (parent 82406fce Engine.cpp:950-1007). Unrouted:
    nothing is touched. */
void Engine::matrixTick (std::int64_t t) noexcept
{
    juce::ignoreUnused (t);
   #if OGI_TEST_HOOKS
    matrix.noResidual    = hooks.noResidual;
    matrix.fullRoundTrip = hooks.fullRoundTrip;
    matrix.noDepthRamp   = hooks.noDepthRamp;
   #endif
    // PD4 / PD5: a live slot with a per-voice source and a class-V
    // destination runs on the per-voice path this cell.
    for (int m = 0; m < kGiModSlots; ++m)
    {
        const auto& w = gp.mod[m];
        pvPath[m] = w.src != GiSrc::None && w.dest != GiDest::None && srcPerVoiceNow (w.src) && destVoiceNow (w.dest);
    }
    matrix.tick (gp, srcG, pvPath);

    // LFO rates of the NEXT cell (global LFOs).
    for (int n = 0; n < kGiLfos; ++n)
    {
        const int d = GiDest::LfoRate1 + n;
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
        traceAt (d, static_cast<float> (lfoRateHz[n]));
    }

    // Stream fields (Position … KeyTrack): the cell's modulated value
    // overwrites the latch (gp is re-latched every tick); tick consumers read
    // it, the spawn re-evaluates at its own sample.
    const bool tempoMode = gp.syncMode == 1;
    for (int s = 0; s < kGiStreams; ++s)
    {
        streamRoutedG[s] = false;
        for (int f = GiDest::Position; f < GiDest::NumFields; ++f)
        {
            const int d = GiDest::stream (s, f);
            if (! matrix.routed (d))
                continue;
            streamRoutedG[s] = true;
            rawBase[d] = matrix.rawNow (d);
            if (tempoMode && (f == GiDest::Size || f == GiDest::Density))
                continue;   // Tempo: the offset scales the synced value (T12)
            GiModMap::streamFieldFromRaw (gp.s[s], f, matrix.apply (d, rawBase[d], matrix.offset (d)));
        }
    }

    // PD5: the per-voice pass, then the pair views (from the raw + offG + offV,
    // one clamp, P3) and the per-voice LFO rates of the next cell. The pair
    // flags come from routed || routedPrev and are derived here, before any
    // dirty check (P8).
    numPairsRoutedPrev = numPairsRouted;
    numPairsRouted = 0;
    int maxDests = 0;
    bool anyCounted = false;
    for (int k = 0; k < kVoiceSlots; ++k)
    {
        if (voices[k].isFree())
        {
            for (int s = 0; s < kGiStreams; ++s)
                pairRoutedK[k][s] = pairRegionOn[k][s] = pairSelOn[k][s] = false;
            continue;
        }
        anyCounted = anyCounted || voices[k].isCounted();
        voicePass (k, false);
        maxDests = std::max (maxDests, vm[k].numRoutedLast);
        buildPairViews (k);
        voiceLfoRates (k);
    }
    writeTelemetry();
   #if OGI_TEST_HOOKS
    hooks.routedPvDests = maxDests;
    if (anyCounted)
        hooks.routedPvDestsMin = std::min (hooks.routedPvDestsMin, maxDests);
   #else
    juce::ignoreUnused (maxDests, anyCounted);
   #endif
}

bool Engine::srcPerVoiceNow (int src) const noexcept
{
    return GiSrc::isPerVoice (src) || (GiSrc::isLfo (src) && lfoPerVoiceActive (src - GiSrc::Lfo1));
}

bool Engine::destVoiceNow (int dest) const noexcept
{
    return GiDest::isVoiceClass (dest) || (GiDest::isLfoRate (dest) && lfoPerVoiceActive (dest - GiDest::LfoRate1));
}

/** PD4: the per-voice routes of voice k into its offsets (with per-voice
    route-change residuals). noteOnSeed: at note-on — the current slot depths,
    prev := cur, so the voice's first grain already carries its modulation. */
void Engine::voicePass (int k, bool noteOnSeed) noexcept
{
    auto& m = vm[k];
    m.beginCell();
    const float* sv = m.src;
    bool noRes = false;
   #if OGI_TEST_HOOKS
    noRes = hooks.noResidual;
    if (hooks.pvSharedSource)
        for (int j = 0; j < kVoiceSlots; ++j)
            if (! voices[j].isFree()) { sv = vm[j].src; break; }   // PV1n: one shared source vector
   #endif
    for (int ms = 0; ms < kGiModSlots; ++ms)
    {
        const auto s = matrix.slot (ms);
        if (! noteOnSeed && s.changed && s.oldPv && ! noRes && ! juce::exactlyEqual (m.lastContrib[ms], 0.0f))
            m.pushResidual (s.oldDest, m.lastContrib[ms], rampCellsMod);
        if (! s.pv)
        {
            m.lastContrib[ms] = 0.0f;
            continue;
        }
        const float c = s.depth * sv[s.src];
        m.lastContrib[ms] = c;
        m.add (s.dest, c);
    }
    if (! noteOnSeed)
        m.tickResiduals();
    m.numRoutedLast = m.finishCell();
    if (noteOnSeed)
        m.seedPrevFromCur();
}

/** PD5 / PD6: the tick view of every pair of voice k that carries a per-voice
    offset (now or in the previous cell). Unrouted pairs read gp.s[s]. */
void Engine::buildPairViews (int k) noexcept
{
    auto& m = vm[k];
    const bool tempoMode = gp.syncMode == 1;
    for (int s = 0; s < kGiStreams; ++s)
    {
        pairRegionOn[k][s] = pairSelOn[k][s] = false;
        pairRoutedK[k][s] = m.pairRouted (s);
        if (! pairRoutedK[k][s])
            continue;
        ++numPairsRouted;
        auto& v = view[k][s];
        v = gp.s[s];
        for (int f = GiDest::Position; f < GiDest::NumFields; ++f)
        {
            const int d = GiDest::stream (s, f);
            const float off = m.offCurAt (d);
            if (! (off > 0.0f || off < 0.0f))
                continue;
            if (tempoMode && (f == GiDest::Size || f == GiDest::Density))
                continue;   // Tempo: the effective values (T12)
            rawBase[d] = matrix.rawNow (d);
            GiModMap::streamFieldFromRaw (v, f, matrix.apply (d, rawBase[d], matrix.offset (d) + off));
        }
        jitKS[k][s] = jitterOf (v);

        // PD6: a per-voice loop region moves only this voice's READ region (P17).
        const SlotTape* tp = current[(size_t) s];
        if (tp != nullptr && (! juce::exactlyEqual (v.loopStart, gp.s[s].loopStart) || ! juce::exactlyEqual (v.loopEnd, gp.s[s].loopEnd)))
        {
            regionV[k][s] = loopRegion (static_cast<double> (v.loopStart), static_cast<double> (v.loopEnd),
                                        tp->frames, kMinLoopMs * 0.001 * tp->rate);
            pairRegionOn[k][s] = true;
        }
        // Per-pair scale window on the stream's ActiveSet (built at the global pitch_rand).
        const PitchSel& gs = scale.selection (s);
        if (gs.set != nullptr && (! juce::exactlyEqual (v.pitchSt, gp.s[s].pitchSt) || ! juce::exactlyEqual (v.pitchRand, gp.s[s].pitchRand)))
        {
            selV[k][s].set = gs.set;
            gs.set->window (static_cast<double> (v.pitchSt), static_cast<double> (v.pitchRand) * 100.0, selV[k][s].lo, selV[k][s].hi);
            pairSelOn[k][s] = true;
        }
    }
}

/** PD12 / R50.5: the rates of voice k's own LFOs for the NEXT cell. */
void Engine::voiceLfoRates (int k) noexcept
{
    auto& m = vm[k];
    const bool tempoMode = gp.syncMode == 1;
    const TempoAnchor& A = tracker.anchor();
    for (int n = 0; n < kGiLfos; ++n)
    {
        if (! m.lfoOn[n])
            continue;
        const int d = GiDest::LfoRate1 + n;
        const float off = matrix.offset (d) + m.offCurAt (d);
        m.lfoRateOff[n] = off;
        if (tempoMode)
            m.lfoRateHz[n] = fs * A.pps / tempoDivBeats (gp.lfo[n].rateSync, A.barLen) * std::exp2 (4.0 * static_cast<double> (off));
        else
            m.lfoRateHz[n] = (off > 0.0f || off < 0.0f) ? static_cast<double> (matrix.apply (d, gp.lfo[n].rateHz, off))
                                                         : static_cast<double> (gp.lfo[n].rateHz);
    }
}

/** Level / pan substitutes of globally routed streams (parent R47 :742-755):
    the base ramp's current value → raw → apply → the engine domain. */
void Engine::applyLevelPanMod() noexcept
{
    bool zero = false;
   #if OGI_TEST_HOOKS
    zero = hooks.mixerRampZero;
   #endif
    for (int s = 0; s < kGiStreams; ++s)
    {
        const auto& sp = gp.s[s];
        if (zero)
            panValRamp[s].setCell (sp.pan, sp.pan);
        else
        {
            panValRamp[s].setTarget (sp.pan, rampCells20);
            panValRamp[s].tick();
        }

        const int dl = GiDest::stream (s, GiDest::Level), dp = GiDest::stream (s, GiDest::Pan);
        const bool rl = matrix.routed (dl), rp = matrix.routed (dp);
        float lv = levelRamp[s].current();
        if (rl)
            lv = levelDbToGain (matrix.apply (dl, GiModMap::levelToRaw (lv), matrix.offset (dl)));
        levelMod[s].setCell (levelNow[s], lv);
        levelNow[s] = lv;
        useLevelMod[s] = rl || matrix.routedPrev (dl);
        traceAt (dl, lv);

        float gl = panLRamp[s].current(), gr = panRRamp[s].current();
        if (rp)
        {
            const float pn = matrix.apply (dp, panValRamp[s].current() * 100.0f, matrix.offset (dp)) / 100.0f;
            const auto pg = panGains (pn);
            gl = pg.a;
            gr = pg.b;
            traceAt (dp, pn);
        }
        else
            traceAt (dp, panValRamp[s].current());
        panLMod[s].setCell (panLNow[s], gl);  panLNow[s] = gl;
        panRMod[s].setCell (panRNow[s], gr);  panRNow[s] = gr;
        usePanMod[s] = rp || matrix.routedPrev (dp);
    }
}

/** PD12 (parent P50, 82406fce Engine.cpp:1013-1033): the synced density as a
    power-of-two multiple of the division clamped to [0.5, 200] g/s, and the
    synced size scaled by 2^(4·off) clamped to [5, 2000] ms. */
void Engine::tempoEffective (float offD, float offS, GiGridParams::Stream& st, double& divOut, double& sizeBaseOut) const noexcept
{
    const TempoAnchor& A = tracker.anchor();
    const double beatsPerS = A.pps * fs;
    const double divBeats = tempoDivBeats (st.densitySync, A.barLen);
    const double d0 = beatsPerS / divBeats;
    const double eLo = std::ceil (std::log2 (kDensityMin / d0)), eHi = std::floor (std::log2 (kDensityMax / d0));
    const double e = std::clamp (std::floor (4.0 * static_cast<double> (offD) + 0.5), eLo, eHi);
    divOut = divBeats * std::exp2 (-e);
    st.density = static_cast<float> (d0 * std::exp2 (e));
    sizeBaseOut = tempoDivBeats (st.sizeSync, A.barLen) * 60000.0 / A.bpm;
    st.sizeMs = static_cast<float> (std::clamp (sizeBaseOut * std::exp2 (4.0 * static_cast<double> (offS)), kMinSizeMs, kMaxSizeMs));
}

/** Tempo, at the tick: the global stream values (global offsets) and every
    routed pair's view (offG + offV). */
void Engine::applyTempoEffective() noexcept
{
    for (int s = 0; s < kGiStreams; ++s)
        tempoEffective (matrix.offset (GiDest::stream (s, GiDest::Density)), matrix.offset (GiDest::stream (s, GiDest::Size)),
                        gp.s[s], tempoDivS[s], tempoSizeBase[s]);
    for (int k = 0; k < kVoiceSlots; ++k)
        if (! voices[k].isFree())
            pairTempo (k);
}

void Engine::pairTempo (int k) noexcept
{
    for (int s = 0; s < kGiStreams; ++s)
    {
        if (! pairRoutedK[k][s])
        {
            tempoDivKS[k][s] = tempoDivS[s];
            continue;
        }
        const int dd = GiDest::stream (s, GiDest::Density), dz = GiDest::stream (s, GiDest::Size);
        double sb = 0.0;
        tempoEffective (matrix.offset (dd) + vm[k].offCurAt (dd), matrix.offset (dz) + vm[k].offCurAt (dz),
                        view[k][s], tempoDivKS[k][s], sb);
    }
}

/** PD7: voice k carries a per-voice level / pan offset on stream s (cur or prev). */
bool Engine::pairLevelPanRouted (int k, int s) const noexcept
{
    const auto& m = vm[k];
    const int dl = GiDest::stream (s, GiDest::Level), dp = GiDest::stream (s, GiDest::Pan);
    const float a = m.offCurAt (dl), b = m.offPrev()[dl], c = m.offCurAt (dp), d = m.offPrev()[dp];
    return a > 0.0f || a < 0.0f || b > 0.0f || b < 0.0f || c > 0.0f || c < 0.0f || d > 0.0f || d < 0.0f;
}

/** PD7: the pair's level / pan of this cell — the stream substitute's base
    (the 2.4 ramp) with offG + offV_k summed before the single clamp. */
void Engine::pairGainTarget (int k, int s, float& gl, float& gr, float& lv) const noexcept
{
    const int dl = GiDest::stream (s, GiDest::Level), dp = GiDest::stream (s, GiDest::Pan);
    const float ol = matrix.offset (dl) + vm[k].offCurAt (dl);
    const float op = matrix.offset (dp) + vm[k].offCurAt (dp);
    lv = (ol > 0.0f || ol < 0.0f) ? levelDbToGain (matrix.apply (dl, GiModMap::levelToRaw (levelRamp[s].current()), ol))
                                  : levelNow[s];
    if (op > 0.0f || op < 0.0f)
    {
        const auto pg = panGains (matrix.apply (dp, panValRamp[s].current() * 100.0f, op) / 100.0f);
        gl = lv * pg.a;
        gr = lv * pg.b;
    }
    else
    {
        gl = lv * panLNow[s];
        gr = lv * panRNow[s];
    }
}

/** PD7 at the tick (after the stream substitutes): which streams mix per pair,
    and every non-free voice's pair gains for this cell. A stream entering the
    mode starts each pair from the stream's previous-cell gains. */
void Engine::updatePairMix() noexcept
{
    int count = 0;
    bool anyCounted = false;
    for (int s = 0; s < kGiStreams; ++s)
    {
        bool on = false;
        for (int k = 0; k < kVoiceSlots && ! on; ++k)
            on = ! voices[k].isFree() && pairLevelPanRouted (k, s);
        const bool entering = on && ! pairMix[s];
        pairMix[s] = on;
        if (! on)
            continue;
        ++count;
        const float lvPrev = levelR (s).previous();
        const float glPrev = lvPrev * panLR (s).previous(), grPrev = lvPrev * panRR (s).previous();
        for (int k = 0; k < kVoiceSlots; ++k)
        {
            if (voices[k].isFree())
                continue;
            anyCounted = anyCounted || voices[k].isCounted();
            float gl = 1.0f, gr = 1.0f, lv = 1.0f;
            pairGainTarget (k, s, gl, gr, lv);
            if (entering)
            {
                pairGL[k][s].reset (glPrev);
                pairGR[k][s].reset (grPrev);
                pairLv[k][s].reset (lvPrev);
            }
            pairGL[k][s].push (gl);
            pairGR[k][s].push (gr);
            pairLv[k][s].push (lv);
        }
    }
   #if OGI_TEST_HOOKS
    hooks.pairModeStreams = count;
    if (anyCounted)
        hooks.pairModeStreamsMin = std::min (hooks.pairModeStreamsMin, count);
   #else
    juce::ignoreUnused (count, anyCounted);
   #endif
}

//==============================================================================
// Events
void Engine::applyEvent (const MidiEvent& e, std::int64_t t) noexcept
{
    if (e.numBytes < 1)
        return;
    nowT = t;
    const int type    = e.status & 0xF0;
    const int channel = (e.status & 0x0F) + 1;

    if (type == 0x90 && e.numBytes >= 3)
    {
        if (e.data2 > 0)
            noteOn (channel, e.data1 & 0x7F, static_cast<float> (e.data2) / 127.0f, t);
        else
            noteOff (channel, e.data1 & 0x7F, t);
    }
    else if (type == 0x80 && e.numBytes >= 3)
    {
        noteOff (channel, e.data1 & 0x7F, t);
    }
    else if (type == 0xE0 && e.numBytes >= 3)
    {
        const int value = (e.data1 & 0x7F) | ((e.data2 & 0x7F) << 7);
        const int d = value - 8192;
        const float b = d >= 0 ? static_cast<float> (d) / 8191.0f : static_cast<float> (d) / 8192.0f;
        mpe.bend[channel] = b;   // PLAN-2.5b PD8: written always, read only with MPE on
        if (! gp.mpeEnable)
        {
            bend = b;
            for (int k = 0; k < kVoiceSlots; ++k)   // PLAN-2.5a PD10: the Bend source (2.5b seam)
                if (! voices[k].isFree())
                    vm[k].bendSrc = bend;
        }
        else if (channel == 1)
            bend = b;                       // D31: the master bend moves the pitch of every note only
        else if (mpe.isMember (channel))
            memberWrite (channel, 0, b);   // member bend: pitch term + Bend source of its notes
    }
    else if (type == 0xB0 && e.numBytes >= 3)
    {
        const int cc = e.data1 & 0x7F, v = e.data2 & 0x7F;
        if (GiMpe::isRpnCc (cc))
        {
            // PLAN-2.5b PD7: the RPN detector is fed whether MPE is on or off.
            const auto r = mpe.feed (channel, cc, v);
            if (r != GiMpe::Rpn::none)
            {
               #if OGI_TEST_HOOKS
                ++hooks.rpnMessages;
               #endif
                if (gp.mpeEnable)
                    for (int k = 0; k < kVoiceSlots; ++k)   // the new range applies to the sounding member notes
                        if (! voices[k].isFree() && mpe.isMember (voices[k].channel))
                            voices[k].mpeSemis = static_cast<double> (mpe.bend[voices[k].channel]) * mpe.memberRange;
            }
        }
        else if (cc == 120)
            allSoundOff();
        else if (cc == 123)
            allNotesOff();
        else if (cc == 64)
        {
            // PLAN-2.5b T5 (R§6): CC64 >= 64 = down; the pedal holds the gate.
            // Under MPE only the master channel's pedal counts (PD8, zone-level).
            if (! gp.mpeEnable || channel == 1)
            {
                const bool down = v >= 64;
                if (down && ! sustainDown)
                    sustainDown = true;
                else if (! down && sustainDown)
                    pedalUp();
            }
        }
        else if (cc == 1)
        {
            wheelMsb = v;   // a CC1 write clears the LSB (14-bit pair, PD10)
            wheelLsb = 0;
        }
        else if (cc == 33)
            wheelLsb = v;
        else if (cc == 74)
        {
            const float sl = std::clamp (static_cast<float> (v - 64) / 64.0f, -1.0f, 1.0f);
            mpe.slide[channel] = sl;
            if (! gp.mpeEnable)
            {
                chanSlide = sl;
                for (int k = 0; k < kVoiceSlots; ++k)
                    if (! voices[k].isFree())
                        vm[k].slide = chanSlide;
            }
            else if (mpe.isMember (channel))
                memberWrite (channel, 2, sl);   // D31: master CC74 ignored under MPE
        }
        else if (cc == 121 && gp.mpeEnable && channel != 1)
        {
            memberReset (channel);   // PD8: a member CC121 resets that channel's expression only
        }
        else if (cc == 121)
        {
            // Reset all controllers (PD10): wheel 0, pressure 0, slide 64, bend 0.
            wheelMsb = wheelLsb = 0;
            chanPressure = 0.0f;
            chanSlide = 0.0f;
            bend = 0.0f;
            for (int k = 0; k < kVoiceSlots; ++k)
                if (! voices[k].isFree())
                {
                    vm[k].pressure = 0.0f;
                    vm[k].slide = 0.0f;
                    vm[k].bendSrc = 0.0f;
                }
            // PLAN-2.5b T5 (RP-015): CC121 also lifts sustain; pending offs apply now.
            if (sustainDown)
                pedalUp();
        }
    }
    else if (type == 0xD0 && e.numBytes >= 2)
    {
        // Channel aftertouch → every voice (non-MPE, any channel; last write wins).
        const float pr = static_cast<float> (e.data1 & 0x7F) / 127.0f;
        mpe.pressure[channel] = pr;
        if (! gp.mpeEnable)
        {
            chanPressure = pr;
            for (int k = 0; k < kVoiceSlots; ++k)
                if (! voices[k].isFree())
                    vm[k].pressure = chanPressure;
        }
        else if (mpe.isMember (channel))
            memberWrite (channel, 1, pr);   // D31: master AT ignored under MPE
    }
    else if (type == 0xA0 && e.numBytes >= 3)
    {
        // Poly aftertouch → the voices on that note (ignored under MPE, D31).
        if (gp.mpeEnable)
            return;
        const int note = e.data1 & 0x7F;
        const float p = static_cast<float> (e.data2 & 0x7F) / 127.0f;
        for (int k = 0; k < kVoiceSlots; ++k)
            if (! voices[k].isFree() && voices[k].note == note)
                vm[k].pressure = p;
    }
}

//==============================================================================
// 2.5b MPE (PLAN-2.5b T9)
/** what: 0 bend, 1 pressure, 2 slide. Writes the voices on `channel`
    (mpeBroadcast hook: every voice, MP1n control). */
void Engine::memberWrite (int channel, int what, float value) noexcept
{
    bool broadcast = false;
   #if OGI_TEST_HOOKS
    broadcast = hooks.mpeBroadcast;
   #endif
    for (int k = 0; k < kVoiceSlots; ++k)
    {
        Voice& v = voices[k];
        if (v.isFree() || (! broadcast && v.channel != channel))
            continue;
        auto& m = vm[k];
        if (what == 0)
        {
            m.bendSrc  = value;
            v.mpeSemis = static_cast<double> (value) * mpe.memberRange;
        }
        else if (what == 1)
            m.pressure = value;
        else
            m.slide = value;
       #if OGI_TEST_HOOKS
        ++hooks.memberWrites;
       #endif
    }
}

void Engine::memberReset (int channel) noexcept
{
    if (channel < 1 || channel > 16)
        return;
    mpe.bend[channel] = mpe.pressure[channel] = mpe.slide[channel] = 0.0f;
    if (! mpe.isMember (channel))
        return;
    memberWrite (channel, 0, 0.0f);
    memberWrite (channel, 1, 0.0f);
    memberWrite (channel, 2, 0.0f);
}

/** MPE on: the voice takes its channel's current member values (MPE spec
    initial values); a non-member note under MPE has no member expression. */
void Engine::applyMemberState (int k) noexcept
{
    if (! gp.mpeEnable)
        return;
    Voice& v = voices[k];
    auto& m = vm[k];
    if (mpe.isMember (v.channel))
    {
        m.bendSrc  = mpe.bend[v.channel];
        m.pressure = mpe.pressure[v.channel];
        m.slide    = mpe.slide[v.channel];
        v.mpeSemis = static_cast<double> (mpe.bend[v.channel]) * mpe.memberRange;
    }
    else
    {
        m.bendSrc = m.pressure = m.slide = 0.0f;
        v.mpeSemis = 0.0;
    }
    m.src[GiSrc::Pressure] = m.pressure;
    m.src[GiSrc::Slide]    = m.slide;
    m.src[GiSrc::Bend]     = m.bendSrc;
}

void Engine::noteOn (int channel, int note, float velocity01, std::int64_t t) noexcept
{
    const int noteId = takeNoteId (channel, note, t);   // PD6 (−1 without a raw NoteOn)

    if (gp.voiceMode != 0)
        noteOnMono (channel, note, velocity01, t, noteId);
    else
    {
        // PD9 (USER): re-striking a pedal-held (channel, note) releases the old
        // voice with its normal tail (JUCE Synthesiser precedent), then the new
        // note allocates normally. Never fires without CC64 (keyDown == gated).
        for (int k = 0; k < kVoiceSlots; ++k)
        {
            const Voice& v = voices[k];
            if (v.isCounted() && v.gated && ! v.keyDown && v.note == note && v.channel == channel)
            {
                releaseVoice (k);
               #if OGI_TEST_HOOKS
                ++hooks.restrikeReleases;
               #endif
            }
        }
        allocateVoice (channel, note, velocity01, t, noteId, glideFromLast());
    }

    lastKeyNote = note;
    lastKey = static_cast<double> (note);
    lastKeySet = true;
    lastKeyNoteId = noteId;
}

double Engine::glideFromLast() const noexcept
{
    return (gp.glideMs > 0.0f && lastKeySet) ? lastKey : std::numeric_limits<double>::quiet_NaN();
}

int Engine::allocateVoice (int channel, int note, float velocity01, std::int64_t t, int noteId, double glideFromKey) noexcept
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
        return -1;

    startVoice (k, channel, note, velocity01, t, noteId, glideFromKey);
    return k;
}

void Engine::startVoice (int k, int channel, int note, float velocity01, std::int64_t t, int noteId, double glideFromKey) noexcept
{
    Voice& v = voices[k];
    v = Voice {};
    v.state     = Voice::State::active;
    v.note      = note;
    v.channel   = channel;
    v.velocity  = velocity01;
    v.gated     = true;
    v.keyDown   = true;    // PLAN-2.5b PD14
    v.serial    = ++serialCounter;
    v.serialNote = note;
    v.noteOnAbs = t;
    v.noteId    = noteId;
    recorder.noteMinted (t);   // PLAN-2.6: Next Note = a serial-minting note event
    if (gp.mpeEnable && mpe.isMember (channel))   // PLAN-2.5b PD11: the member bend at note-on
        v.mpeSemis = static_cast<double> (mpe.bend[channel]) * mpe.memberRange;
   #if OGI_TEST_HOOKS
    if (hooks.neModuleTable && note >= 0 && note < 128)
    {
        v.neSemis = neTable[note];   // applyPendingTuning: exchange-to-0 at note-on
        neTable[note] = 0.0;
    }
   #endif
    // PLAN-2.5b PD13: a fresh voice glides from the last key (Poly with glide
    // > 0, or Mono / Legato with no cloud); NaN / glide 0 → no glide state.
    if (! std::isnan (glideFromKey))
    {
        bool step = false;
       #if OGI_TEST_HOOKS
        step = hooks.glideStep;
       #endif
        if (gp.glideMs > 0.0f && ! step)
        {
            v.glFrom = glideFromKey;
            v.glTo   = static_cast<double> (note) + v.neSemis;
            v.glT0   = t;
            v.glN    = static_cast<double> (gp.glideMs) * 0.001 * fs;
        }
    }
   #if OGI_TEST_HOOKS
    ++hooks.serialsMinted;
   #endif
    v.env.noteOn (ampParams(), fs, AmpEnv::velocityGain (velocity01, gp.velToAmp));
    initVoiceMod (k);   // PLAN-2.5a PD10 / PD11: per-voice sources
    for (int s = 0; s < kGiStreams; ++s)
    {
        v.sched[s].reset();                    // first onset exactly at the note-on sample
        {
            // PD1 (USER): Tempo → one burst onset at the note-on sample, then the
            // grid strictly after it; tempoGridOnly → the parent R41 (grid only).
            bool gridOnly = false;
           #if OGI_TEST_HOOKS
            gridOnly = hooks.tempoGridOnly;
           #endif
            const bool burst = gp.syncMode == 1 && ! gridOnly && gp.s[s].enable;
            tclk[k][s].restart (burst ? t + 1 : -1);
            burstPending[k][s] = burst;
            burstCount[k][s] = 0;
            divKS[k][s] = pairRoutedK[k][s] ? tempoDivKS[k][s] : tempoDivS[s];
        }
        v.scan[s].restart (t, static_cast<double> (pairSp (k, s).scanRate) / fs);
    }

    // Until the next grid tick includes it, the new voice runs unthinned.
    for (int s = 0; s < kGiStreams; ++s)
    {
        quotaM[k][s]  = kQuotaUncapped;
        const auto& spk = pairSp (k, s);
        periodT[k][s] = uncappedPeriod (spk);
        const double n = streamDemand (static_cast<double> (spk.density), static_cast<double> (spk.sizeMs), fs);
        lawTarget[k][s] = pairLaw (k, s, n);
        lawGain[k][s].reset (lawTarget[k][s]);
        gFbTarget[k][s] = pairFbGain (k, s, n);   // PLAN-2.4 PD8
        gFbLerp[k][s].reset (gFbTarget[k][s]);
    }
    quotasDirty = true;

    writeVoiceHooks (k);
}

void Engine::noteOff (int channel, int note, std::int64_t t) noexcept
{
    if (gp.voiceMode != 0)
    {
        noteOffMono (channel, note, t);
        return;
    }
    for (int k = 0; k < kVoiceSlots; ++k)
    {
        Voice& v = voices[k];
        if (v.isKeyDown() && v.note == note && v.channel == channel)
        {
            if (sustainDown)
            {
                // PLAN-2.5b T5: the pedal holds the gate; applied at pedal-up.
                v.keyDown = false;
               #if OGI_TEST_HOOKS
                ++hooks.sustainDeferred;
               #endif
            }
            else
                releaseVoice (k);
        }
    }
}

/** Gate close: amp env + both Mod Envs release from their current level. */
void Engine::releaseVoice (int k) noexcept
{
    Voice& v = voices[k];
    if (! v.isCounted() || ! v.gated)
        return;
    v.gated = false;
    v.keyDown = false;
    v.env.noteOff (ampParams(), fs);
    auto& m = vm[k];
    for (int e = 0; e < kGiModEnvs; ++e)
        m.env[e].noteOff (menvParams (e), fs);
}

/** CC64 up (or CC121): every pedal-held voice (gate open, no key down)
    releases at this sample. Hook pedalUpHard: a 0 ms cut (CK5en control). */
void Engine::pedalUp() noexcept
{
    sustainDown = false;
    bool hard = false;
   #if OGI_TEST_HOOKS
    hard = hooks.pedalUpHard;
   #endif
    for (int k = 0; k < kVoiceSlots; ++k)
    {
        const Voice& v = voices[k];
        if (v.isCounted() && v.gated && ! v.keyDown)
        {
            if (hard)
                forceReleaseVoiceMs (k, 0.0);
            else
                releaseVoice (k);
        }
    }
}

void Engine::allNotesOff() noexcept
{
    // CC123 releases every open gate and IGNORES the pedal (P10: the
    // transport-stop safety); the Mono stack is cleared.
    for (int k = 0; k < kVoiceSlots; ++k)
        if (voices[k].isCounted() && voices[k].gated)
            releaseVoice (k);
    noteStack.clear();
    detached = false;
}

void Engine::allSoundOff() noexcept
{
    for (int k = 0; k < kVoiceSlots; ++k)
        if (voices[k].isCounted())
            forceReleaseVoice (k);
    noteStack.clear();
    detached = false;
}

void Engine::forceReleaseVoice (int k) noexcept
{
    double ms = kStealMs;
   #if OGI_TEST_HOOKS
    if (hooks.stealRampZero)
        ms = 0.0;
   #endif
    forceReleaseVoiceMs (k, ms);
}

void Engine::forceReleaseVoiceMs (int k, double ms) noexcept
{
    Voice& v = voices[k];
    if (v.isFree() || v.isStolen())
        return;
    v.state = Voice::State::stolen;
    v.gated = false;
    v.keyDown = false;
    v.env.forceRelease (fs, ms);
    quotasDirty = true;
    writeVoiceHooks (k);
}

//==============================================================================
// 2.5b voice modes (PLAN-2.5b T6 / T7)
int Engine::monoCloud() const noexcept
{
    int best = -1;
    for (int k = 0; k < kVoiceSlots; ++k)
        if (voices[k].isCounted() && (best < 0 || voices[k].serial > voices[best].serial))
            best = k;
    return best;
}

/** PD13: glide from the voice's CURRENT key (note + ne + g(t)) to toKey;
    retarget mid-glide is continuous at t. glide 0 / glideStep → no glide. */
void Engine::setGlide (Voice& v, double toKey, std::int64_t t) noexcept
{
    const double cur = static_cast<double> (v.note) + v.neSemis + v.glideAt (t);
    bool step = false;
   #if OGI_TEST_HOOKS
    step = hooks.glideStep;
   #endif
    if (gp.glideMs > 0.0f && ! step)
    {
        v.glFrom = cur;
        v.glTo   = toKey;
        v.glT0   = t;
        v.glN    = static_cast<double> (gp.glideMs) * 0.001 * fs;
    }
    else
        v.glN = 0.0;
}

/** R§2.1 + PD2 / PD5. The cloud = the max-serial counted voice. */
void Engine::noteOnMono (int channel, int note, float velocity01, std::int64_t t, int noteId) noexcept
{
    GiNoteEntry e;
    e.channel = channel;
    e.note    = note;
    e.noteId  = noteId;
    e.neSemis = 0.0;

    const int c = monoCloud();
    const bool held = c >= 0 && ! detached && voices[c].gated && voices[c].keyDown;
    noteStack.push (e);
   #if OGI_TEST_HOOKS
    hooks.stackDropped = noteStack.dropped;
   #endif

    bool legatoAsMono = false;
   #if OGI_TEST_HOOKS
    legatoAsMono = hooks.legatoAsMono;
   #endif
    if (held)
    {
        if (gp.voiceMode == 2 && ! legatoAsMono)
        {
            moveCloudPitch (c, e, t);   // D29 Legato: same serial, pitch only
           #if OGI_TEST_HOOKS
            ++hooks.legatoMoves;
           #endif
        }
        else
            retriggerVoice (c, e, velocity01, t);   // D29 Mono
    }
    else if (c >= 0)
        retriggerVoice (c, e, velocity01, t);       // PD2 / PD5: no key held → full retrigger on the cloud's slot
    else
        allocateVoice (channel, note, velocity01, t, noteId, glideFromLast());
    detached = false;
}

void Engine::noteOffMono (int channel, int note, std::int64_t t) noexcept
{
    const bool wasTop  = noteStack.isTop (channel, note);
    const bool removed = noteStack.remove (channel, note);
    const int c = monoCloud();
    if (c < 0)
        return;
    if (detached)
    {
        // PD5: a held key's release never releases the detached cloud (the
        // pedal governs it); the flag clears when no physical key is left.
        if (noteStack.empty())
            detached = false;
        return;
    }
    if (! removed)
        return;   // not a stack key (overflow-dropped)
    Voice& v = voices[c];
    if (! v.keyDown)
        return;
    if (wasTop && ! noteStack.empty())
    {
        // D30: glide back to the new top, no retrigger (both modes).
        const GiNoteEntry top = *noteStack.top();
        bool retrig = false;
       #if OGI_TEST_HOOKS
        retrig = hooks.fallbackRetrig;
        ++hooks.fallbacks;
       #endif
        if (retrig)
            retriggerVoice (c, top, v.velocity, t);
        else
            moveCloudPitch (c, top, t);
    }
    else if (noteStack.empty())
    {
        if (sustainDown)
        {
            v.keyDown = false;   // the gate holds until pedal-up
           #if OGI_TEST_HOOKS
            ++hooks.sustainDeferred;
           #endif
        }
        else
            releaseVoice (c);
    }
}

/** D29 Legato / D30 fallback: the target note moves (glide), same serial;
    the Key source follows the target (PD3); nothing retriggers. */
void Engine::moveCloudPitch (int k, const GiNoteEntry& e, std::int64_t t) noexcept
{
    Voice& v = voices[k];
   #if OGI_TEST_HOOKS
    if (recorder.hooks.nnAnyNote)
        recorder.noteMinted (t);   // MN1n: a Legato pitch move (same serial) counted as Next Note
   #endif
    setGlide (v, static_cast<double> (e.note) + e.neSemis, t);
    v.note    = e.note;
    v.channel = e.channel;
    v.noteId  = e.noteId;
    v.neSemis = e.neSemis;
    v.keyDown = true;
    vm[k].src[GiSrc::Key] = std::clamp (static_cast<float> (e.note - 60) / 64.0f, -1.0f, 1.0f);
    applyMemberState (k);   // PD8: member expression follows the sounding key
    quotasDirty = true;
    writeVoiceHooks (k);
}

/** D29 Mono (R§2.2): a NEW serial on the same slot; amp env + Mod Envs from
    the current level (PD1 gain glide), scan / schedulers / Tempo restart,
    per-voice LFOs at lfoN_phase, NR / velocity / Key updated, PD4 residuals.
    Never startVoice (P2: it snaps lawGain / gFbLerp). */
void Engine::retriggerVoice (int k, const GiNoteEntry& e, float velocity01, std::int64_t t) noexcept
{
    Voice& v = voices[k];
    bool viaStart = false, resetZero = false, snap = false;
   #if OGI_TEST_HOOKS
    viaStart  = hooks.retrigViaStartVoice;
    resetZero = hooks.ampResetZero;
    snap      = hooks.gainSnap;
    ++hooks.retriggers;
   #endif
    if (viaStart)
    {
        // VM5n control: the Poly note-on path on the sounding slot.
        const double from = static_cast<double> (v.note) + v.neSemis + v.glideAt (t);
        startVoice (k, e.channel, e.note, velocity01, t, e.noteId, gp.glideMs > 0.0f ? from : std::numeric_limits<double>::quiet_NaN());
        voices[k].neSemis = e.neSemis;
        return;
    }

    setGlide (v, static_cast<double> (e.note) + e.neSemis, t);
    v.note       = e.note;
    v.channel    = e.channel;
    v.noteId     = e.noteId;
    v.neSemis    = e.neSemis;
    v.velocity   = velocity01;
    v.gated      = true;
    v.keyDown    = true;
    v.serial     = ++serialCounter;
    v.serialNote = e.note;
    v.noteOnAbs  = t;
    recorder.noteMinted (t);   // PLAN-2.6: a Mono retrigger mints → Next Note
   #if OGI_TEST_HOOKS
    ++hooks.serialsMinted;
   #endif
    const float gv = AmpEnv::velocityGain (velocity01, gp.velToAmp);
    if (resetZero)
        v.env.noteOn (ampParams(), fs, gv);
    else
        v.env.retrigger (ampParams(), fs, gv, snap ? 0.0 : kStealMs);

    applyMemberState (k);   // PD8: member expression follows the sounding key
    retrigVoiceMod (k);

    for (int s = 0; s < kGiStreams; ++s)
    {
        v.epoch[s] = 0;              // new serial: the counter space is fresh
        v.sched[s].reset();          // first onset at the retrigger sample
        bool gridOnly = false;
       #if OGI_TEST_HOOKS
        gridOnly = hooks.tempoGridOnly;
       #endif
        const bool burst = gp.syncMode == 1 && ! gridOnly && gp.s[s].enable;
        tclk[k][s].restart (burst ? t + 1 : -1);
        burstPending[k][s] = burst;
        burstCount[k][s] = 0;
        divKS[k][s] = pairRoutedK[k][s] ? tempoDivKS[k][s] : tempoDivS[s];
        v.scan[s].restart (t, static_cast<double> (pairSp (k, s).scanRate) / fs);
    }
    quotasDirty = true;
    writeVoiceHooks (k);
}

/** The per-voice source part of a Mono retrigger (initVoiceMod minus the
    clear and the pair-mixer snap): Mod Envs retrigger from their level, the
    per-voice LFOs re-prepare at lfoN_phase, NR / velocity / Key are new; per-
    voice level / pan routes get a 20 ms residual (PD4). */
void Engine::retrigVoiceMod (int k) noexcept
{
    auto& m = vm[k];
    const Voice& v = voices[k];
    bool resetZero = false, noRes = false, slotKey = false;
   #if OGI_TEST_HOOKS
    resetZero = hooks.ampResetZero;
    noRes     = hooks.noPvResidual;
    slotKey   = hooks.slotKeyedSeed;
   #endif
    float oldC[kGiModSlots];
    std::copy (std::begin (m.lastContrib), std::end (m.lastContrib), oldC);

    for (int e = 0; e < kGiModEnvs; ++e)
    {
        if (resetZero)
            m.env[e].noteOn (menvParams (e), fs);
        else
            m.env[e].retrigger (menvParams (e), fs);
    }

    const bool tempoMode = gp.syncMode == 1;
    const TempoAnchor& A = tracker.anchor();
    for (int n = 0; n < kGiLfos; ++n)
    {
        m.lfoRateOff[n] = 0.0f;
        m.lfoRateHz[n] = tempoMode ? fs * A.pps / tempoDivBeats (gp.lfo[n].rateSync, A.barLen)
                                   : static_cast<double> (gp.lfo[n].rateHz);
        m.lfoOn[n] = false;
        if (! lfoPerVoiceActive (n))
            continue;
        m.lfo[n].prepare (fs, rampCellsMod, gp.lfo[n].phaseDeg);
        m.lfoOn[n] = true;
    }

    const std::uint64_t key = slotKey ? static_cast<std::uint64_t> (k) : v.serial;
    for (int n = 0; n < kGiLfos; ++n)
    {
        if (! m.lfoOn[n])
        {
            m.src[GiSrc::Lfo1 + n] = srcG[GiSrc::Lfo1 + n];
            continue;
        }
        double ph = static_cast<double> (m.lfo[n].getPhaseDeg()) / 360.0 + m.lfo[n].getFrac();
        ph -= std::floor (ph);
        const float pv = Lfo::shape (gp.lfo[n].shape, ph, m.lfo[n].getCycle(), voiceLfoSeed (key), n);
        const float xf = lfoXf[n].current();
        m.src[GiSrc::Lfo1 + n] = (1.0f - xf) * srcG[GiSrc::Lfo1 + n] + xf * pv;
    }
    m.src[GiSrc::ModEnv1]    = static_cast<float> (m.env[0].getLevel());
    m.src[GiSrc::ModEnv2]    = static_cast<float> (m.env[1].getLevel());
    m.src[GiSrc::Velocity]   = v.velocity;
    m.src[GiSrc::Key]        = std::clamp (static_cast<float> (v.note - 60) / 64.0f, -1.0f, 1.0f);
    m.src[GiSrc::NoteRandom] = ouaricon::grain::CounterRng::Draw (seed, kNoteRandStream, key).nextFloat (0);

    // PD4: per-voice level / pan routes carry (old − new) over 20 ms.
    if (! noRes)
        for (int ms = 0; ms < kGiModSlots; ++ms)
        {
            const auto sl = matrix.slot (ms);
            if (! sl.pv || ! GiDest::isStream (sl.dest))
                continue;
            const int f = GiDest::fieldOf (sl.dest);
            if (f != GiDest::Level && f != GiDest::Pan)
                continue;
            const float d = oldC[ms] - sl.depth * m.src[sl.src];
            if (d > 0.0f || d < 0.0f)
                m.pushResidual (sl.dest, d, rampCellsMod);
        }

    voicePass (k, true);
    buildPairViews (k);
    if (gp.syncMode == 1)
        pairTempo (k);
    voiceLfoRates (k);
}

/** D32 at the tick where the latched voice_mode changes. Poly → Mono /
    Legato: the newest counted voice is kept, the rest force-release (5 ms);
    the stack is rebuilt from the physical keys (oldest serial at the
    bottom); PD5 detached cloud. Other switches: no voice action. */
void Engine::modeSwitch() noexcept
{
    const int mode = gp.voiceMode;
    if (mode == prevVoiceMode)
        return;
   #if OGI_TEST_HOOKS
    ++hooks.modeSwitches;
   #endif
    if (prevVoiceMode == 0)
    {
        double ms = kStealMs;
       #if OGI_TEST_HOOKS
        if (hooks.modeSwitchHard)
            ms = 0.0;
       #endif
        int order[kVoiceSlots] {};
        const int n = VoiceAllocator::countedOldestFirst (voices, order);
        noteStack.clear();
        for (int i = 0; i < n; ++i)
        {
            const Voice& v = voices[order[i]];
            if (v.keyDown)
                noteStack.push ({ v.channel, v.note, v.noteId, v.neSemis });
        }
        for (int i = 0; i + 1 < n; ++i)
            forceReleaseVoiceMs (order[i], ms);
        detached = n > 0 && ! voices[order[n - 1]].keyDown && ! noteStack.empty();
    }
    else if (mode == 0)
    {
        noteStack.clear();
        detached = false;
    }
    prevVoiceMode = mode;
}

/** PD6: the first unconsumed raw NoteOn of this block at the same
    (offset, channel, pitch) → its noteId; −1 when none (AU / Standalone). */
int Engine::takeNoteId (int channel, int note, std::int64_t t) noexcept
{
    const std::int64_t off = t - neBlockStart;
    for (int j = 0; j < numNeOn; ++j)
    {
        NeOn& r = neOn[j];
        if (! r.used && r.offset == off && r.channel == channel && r.pitch == note)
        {
            r.used = true;
            return r.noteId;
        }
    }
    return -1;
}

/** PD6: a kTuningTypeID value → semis = 240·(value − 0.5), written to every
    non-free voice and stack entry with that noteId (noteId −1 ignored). The
    pitch is latched per grain, so only grains spawned from this sample move. */
void Engine::applyNe (const GiNeEvent& e) noexcept
{
    if (e.type != 2 || e.typeId != kTuningTypeID || e.noteId < 0)
        return;
    const double semis = 240.0 * (e.value - 0.5);
    bool hit = false;
    for (auto& v : voices)
        if (! v.isFree() && v.noteId == e.noteId)
        {
            // A running glide keeps its start: the target moves with the tuning
            // (g is relative to glTo = note + ne), so it ends on the tuned pitch.
            if (v.glN > 0.0)
                v.glTo += semis - v.neSemis;
            v.neSemis = semis;
            hit = true;
        }
    for (int i = 0; i < noteStack.size(); ++i)
        if (noteStack.at (i).noteId == e.noteId)
        {
            noteStack.at (i).neSemis = semis;
            hit = true;
        }
    if (lastKeySet && lastKeyNoteId == e.noteId)
        lastKey = static_cast<double> (lastKeyNote) + semis;
   #if OGI_TEST_HOOKS
    if (hit)
        ++hooks.neApplied;
   #else
    juce::ignoreUnused (hit);
   #endif
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
    {
        quotaM[k][s] = kQuotaUncapped;
        pairRoutedK[k][s] = pairRegionOn[k][s] = pairSelOn[k][s] = false;
    }
    quotasDirty = true;
    writeVoiceHooks (k);
    if (detached && monoCloud() < 0)
        detached = false;   // PD5: cleared by the cloud's end
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
    // PLAN-2.5a PD11: the Mod Envs advance over the same segment splits
    // (sampled at the tick). They never end a voice.
    {
        bool live = false;
       #if OGI_TEST_HOOKS
        live = hooks.envLiveTimes;
       #endif
        const ModEnvParams mp[kGiModEnvs] { menvParams (0), menvParams (1) };
        for (int k = 0; k < kVoiceSlots; ++k)
            if (! voices[k].isFree())
                for (int m = 0; m < kGiModEnvs; ++m)
                    vm[k].env[m].advance (len, mp[m], fs, live);
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
    const bool tempo = gp.syncMode == 1;
    const TempoAnchor& A = tracker.anchor();
    const std::uint64_t lock = tracker.getRelockEpoch();
    bool rederive = false;
   #if OGI_TEST_HOOKS
    rederive = hooks.ceilPerTick && (t0 & static_cast<std::int64_t> (kCell - 1)) == 0;
   #endif
    for (int i = 0; i < nv; ++i)
    {
        const int k = order[i];
        Voice& v = voices[k];
        for (int s = 0; s < kGiStreams; ++s)
        {
            if (! gp.s[s].enable)
            {
                burstPending[k][s] = false;
                continue;
            }
            const double T = periodT[k][s];
            const double J = pairRoutedK[k][s] ? jitKS[k][s] : jit[s];
            const std::uint64_t serial = v.serial, ep = v.epoch[s];
            if (tempo)
            {
                // PD1 (USER): one burst onset at the note-on sample (this
                // segment's start), on the reserved counter kBurstBit | n …
                if (burstPending[k][s])
                {
                    merged[numOnsets++] = { k, s, 0, 0.0, 0, burstCount[k][s]++, serial, true };
                    burstPending[k][s] = false;
                   #if OGI_TEST_HOOKS
                    ++hooks.burstOnsets;
                   #endif
                }
                // … then the bar-anchored grid (PD12, the parent processTempo port).
                const int c = tclk[k][s].process (len, t0, A, divKS[k][s], lock, rederive, J,
                    [this, s, serial, ep] (std::uint64_t idx) noexcept { return jitterU (s, serial, ep, idx); }, tOnBuf);
                for (int j = 0; j < c; ++j)
                    merged[numOnsets++] = { k, s, tOnBuf[j].offset, tOnBuf[j].frac, tOnBuf[j].index, ep + tOnBuf[j].index, serial, false };
               #if OGI_TEST_HOOKS
                hooks.tempoOnsets += static_cast<std::uint64_t> (c);
               #endif
                continue;
            }
            const auto res = v.sched[s].process (len, onsetBuf[k][s], kOnsetCap,
                [this, T, J, s, serial, ep] (std::uint64_t idx) noexcept { return jitterGap (T, J, s, serial, ep, idx); });
            for (int j = 0; j < res.count; ++j)
            {
                const auto& o = onsetBuf[k][s][j];
                merged[numOnsets++] = { k, s, o.offset, o.frac, o.index, ep + o.index, serial, false };
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
       #if OGI_TEST_HOOKS
        if (hooks.headReadScan)
            scanGrainVsHead (vg, slot, c0, c1);
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

#if OGI_TEST_HOOKS
/** headReadScan (PLAN-2.4 PD9): the exact interval test of the grain's linear
    read path vs the running head over this segment (live slots only). */
void Engine::scanGrainVsHead (const VoiceGrain& vg, int slot, int c0, int c1) noexcept
{
    if (slot < 0 || slot >= kPoolLive || vg.tape == nullptr)
        return;
    const int s = std::clamp (vg.slot, 0, kTapeSlots - 1);
    const RecordHead& hd = head[s];
    if (hd.parked || vg.tape != current[(size_t) s] || vg.wrapW > 0.0)
        return;   // no writes this cell / another tape / the wrap-read branch (forced)
    const Grain& g = vg.g;
    const double remain = g.len - g.frac - static_cast<double> (g.age);
    if (! (remain > 0.0))
        return;
    const int n = std::min (c1 - c0, static_cast<int> (std::ceil (remain)));
    if (n <= 0)
        return;
    const double h0 = static_cast<double> (hd.headAt (c0));
    const double d0 = g.readAbs - h0;
    const double d1 = (g.readAbs + g.step * static_cast<double> (n - 1)) - (h0 + static_cast<double> (n - 1));
    std::int64_t rlo = 0, rhi = 0;
    RecordHead::latchRegion (region[s].lo, region[s].hi, vg.tape->frames, rlo, rhi);
    const bool circular = rlo == hd.lo && rhi == hd.hi;
    const bool hit = headPathTouches (d0, d1, g.level, static_cast<double> (hd.hi - hd.lo), circular);
    if (hooks.spawnedRunning[slot] && ! hooks.spawnedForced[slot])
    {
        ++hooks.headScanSegments;
        if (hit) ++hooks.headScanViolations;
    }
    else if (! hooks.spawnedRunning[slot] && hit)
        ++hooks.parkedSpawnCrossings;
}
#endif

void Engine::spawn (const PendingOnset& o, std::int64_t tAbs) noexcept
{
    Voice& v = voices[o.voice];
    if (! v.isCounted() || v.serial != o.serial)
        return;   // ended / stolen inside this segment

    const int s = std::clamp (o.stream, 0, kGiStreams - 1);

    // PLAN-2.5a PD5 / P54 (parent 82406fce Engine.cpp:1155-1182): routed
    // spawn-latched fields are evaluated AT the spawn sample — the offset
    // lerped prev → cur at the onset's cell position — into a local copy built
    // BEFORE any draw; unrouted: &gp.s[s] (the 2.4 path, bitwise).
    const GiGridParams::Stream* spp = &gp.s[s];
    GiGridParams::Stream local;
    const bool pairR = pairRoutedK[o.voice][s];
    if (streamRoutedG[s] || pairR)
    {
        local = pairR ? view[o.voice][s] : gp.s[s];
        const double at = static_cast<double> (tAbs & static_cast<std::int64_t> (kCell - 1));
        const bool tempoMode = gp.syncMode == 1;
        const auto& mv = vm[o.voice];
        for (int f = GiDest::Position; f < GiDest::NumFields; ++f)
        {
            if (f == GiDest::ScanRate || f == GiDest::LoopStart || f == GiDest::LoopEnd)
                continue;   // tick-only (scan anchor, loop region)
            const int d = GiDest::stream (s, f);
            const bool gR = matrix.routed (d);
            const float vp = pairR ? mv.offPrev()[d] : 0.0f, vc = pairR ? mv.offCurAt (d) : 0.0f;
            const bool vR = vp > 0.0f || vp < 0.0f || vc > 0.0f || vc < 0.0f;
            if (! gR && ! vR)
                continue;
            // offG + offV at the spawn sample, summed BEFORE the single clamp (P3).
            const float off = (gR ? matrix.offsetAt (d, at) : 0.0f) + (vR ? mv.offsetAt (d, at) : 0.0f);
            if (tempoMode && f == GiDest::Density)
                continue;   // Tempo: the effective density (tick)
            if (tempoMode && f == GiDest::Size)
            {
                // parent P54: the synced size scaled by 2^(4·off) at the spawn sample
                local.sizeMs = static_cast<float> (std::clamp (tempoSizeBase[s] * std::exp2 (4.0 * static_cast<double> (off)),
                                                               kMinSizeMs, kMaxSizeMs));
                continue;
            }
            if (vR && ! gR)
                rawBase[d] = matrix.rawNow (d);
            GiModMap::streamFieldFromRaw (local, f, matrix.apply (d, rawBase[d], off));
        }
        spp = &local;
    }
    const auto& sp = *spp;
   #if OGI_TEST_HOOKS
    {
        const int td = hooks.traceDest.dest;
        if (GiDest::isStream (td) && GiDest::streamOf (td) == s && GiDest::fieldOf (td) >= GiDest::Position
            && (hooks.traceDest.voice < 0 || hooks.traceDest.voice == o.voice))
            traceAt (td, GiModMap::streamFieldValue (sp, GiDest::fieldOf (td)));
    }
   #endif
    SlotTape* tp = current[(size_t) s];   // stream s reads slot s
    if (! sp.enable || tp == nullptr || tp->frames <= 0)
        return;

    // Draws BEFORE acquire (parent rule): a refused or stolen spawn consumes the
    // same draws. Keyed on the note serial and the per-(k,s) counter index
    // epoch + onsetIndex (PD13), never the voice slot. Every draw goes through
    // its fixed Slot; a randomisation at 0 draws nothing.
    const std::uint64_t counter = o.burst ? (kBurstBit | (v.serial << 24) | (o.idx & 0xFFFFFFull))   // PD1: disjoint from every grid counter
                                          : (v.serial << 24) | (o.idx & 0xFFFFFFull);
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
    double st = voiceSemis (v, sp, tAbs);
    const PitchSel& sel = pairSel (o.voice, s);
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
    const LoopRegion& rg = pairRegion (o.voice, s);   // PD6: a per-voice READ region
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

    // PLAN-2.4 PD9: record-head clamp. No draws, skipped while the head is
    // parked (fb 0 / frozen / no voice): every derandomised row is untouched.
    bool headRunning = false, clampForced = false;
    {
        const RecordHead& hd = head[s];
        bool off = false;
       #if OGI_TEST_HOOKS
        off = hooks.headClampOff;
       #endif
        headRunning = ! hd.parked;
        if (headRunning && ! off)
        {
            const double hpos = static_cast<double> (hd.headAt (static_cast<int> (tAbs & static_cast<std::int64_t> (kCell - 1))));
            const double Wh = static_cast<double> (hd.hi - hd.lo);
            std::int64_t rlo = 0, rhi = 0;
            RecordHead::latchRegion (rg.lo, rg.hi, tp->frames, rlo, rhi);
            const bool circular = rlo == hd.lo && rhi == hd.hi && hpos >= static_cast<double> (hd.lo)
                               && hpos < static_cast<double> (hd.hi);
            if (wW > 0.0)
                clampForced = true;   // reach >= W: the wrap-read branch is always infeasible
            else
            {
                const double fitLo = dir > 0 ? rg.lo : rg.lo + reach;
                const double fitHi = dir > 0 ? rg.hi - reach : rg.hi;
                const auto res = clampToHead (p0, dir, rate, len, level, fitLo, fitHi, Wh, circular, hpos);
                p0 = res.p0;
                clampForced = res.forced;
               #if OGI_TEST_HOOKS
                if (res.moved) ++hooks.clampMoves;
               #endif
            }
           #if OGI_TEST_HOOKS
            if (clampForced) ++hooks.forcedCrossings;
           #endif
        }
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
    if (a.slot >= 0 && a.slot < kPoolLive)
    {
        hooks.spawnedRunning[a.slot] = headRunning;
        hooks.spawnedForced[a.slot]  = clampForced;
    }
   #else
    juce::ignoreUnused (headRunning, clampForced);
   #endif

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
        entry.counterIdx  = o.burst ? (kBurstBit | o.idx) : o.idx;
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

/** PLAN-2.4 PD7 / PD8: per stream the mean tap
        tap = (Σ_k a_k·g_fb,kS·bus_kS) · min (level, 1) · aud / max (1, Σ_k a_k)
    (no pan, no g_out, no voice gain: parent R31) → chain[s] (unity knob, or
    the parent's 0.95·feedback under chainKnobParity) → fbRet[s][·][c]. */
void Engine::runChains (int c0, int c1, std::int64_t tAbs) noexcept
{
    const int len = c1 - c0;
    bool parity = false, sumT = false;
   #if OGI_TEST_HOOKS
    parity = hooks.chainKnobParity;
    sumT   = hooks.sumTap;
   #endif
    for (int s = 0; s < kGiStreams; ++s)
    {
        for (int c = c0; c < c1; ++c)
        {
            const float lv = pairMix[s] ? audibleRamp[s].value (c)   // PD7: per-pair level already in the tap
                                        : std::min (levelR (s).value (c), 1.0f) * audibleRamp[s].value (c);
            float l = tapAcc[s][0][c] * lv;
            float r = tapAcc[s][1][c] * lv;
            if (! sumT)
            {
                const float d = std::max (1.0f, envSum[s][c]);
                l /= d;
                r /= d;
            }
            tapBuf[0][c - c0] = l;
            tapBuf[1][c - c0] = r;
        }
       #if OGI_TEST_HOOKS
        if (hooks.tapProbe.buf != nullptr && hooks.tapProbe.slot == s)
            for (int i = 0; i < len; ++i)
                if (tAbs + i >= 0 && tAbs + i < hooks.tapProbe.len)
                {
                    hooks.tapProbe.buf[2 * (tAbs + i)]     = tapBuf[0][i];
                    hooks.tapProbe.buf[2 * (tAbs + i) + 1] = tapBuf[1][i];
                }
        {
            auto& inj = hooks.injectFbNan;
            if (inj.slot == s && inj.atSample >= 0 && ! inj.done && tAbs + len > inj.atSample)
            {
                chain[s].testInjectAt  = static_cast<int> (std::max<std::int64_t> (0, inj.atSample - tAbs));
                chain[s].testInjectNan = true;
                inj.done = true;
            }
        }
       #endif
        chain[s].process (tapBuf[0], tapBuf[1], len, c0, parity ? parityKnob : unityKnob,
                          useDiffMod ? diffMod : diffRamp, useSatMod ? satDMod : satDRamp);
       #if OGI_TEST_HOOKS
        chain[s].testInjectAt = -1;
        if (hooks.retProbe.buf != nullptr && hooks.retProbe.slot == s)
            for (int i = 0; i < len; ++i)
                if (tAbs + i >= 0 && tAbs + i < hooks.retProbe.len)
                {
                    hooks.retProbe.buf[2 * (tAbs + i)]     = tapBuf[0][i];
                    hooks.retProbe.buf[2 * (tAbs + i) + 1] = tapBuf[1][i];
                }
       #else
        juce::ignoreUnused (tAbs);
       #endif
        for (int i = 0; i < len; ++i)
        {
            fbRet[s][0][c0 + i] = tapBuf[0][i];
            fbRet[s][1][c0 + i] = tapBuf[1][i];
        }
    }
}

RecordHeadCounters& Engine::headCounters (int s) noexcept
{
   #if OGI_TEST_HOOKS
    return hooks.head[s];
   #else
    return headCntShip[s];
   #endif
}

/** PLAN-2.4 PD4 (at the tick, before takeTapes): the λ ramp, freeze edges, the
    per-slot run condition
        run = ! freeze && rate == fs && ! xfade && ! pending
              && ((enabled && audible && a counted voice) || returnPk > −100 dBFS)
    and the park transition (λ_eff settled at exactly 0 → settle, parked). */
void Engine::updateHeads (std::int64_t t) noexcept
{
    juce::ignoreUnused (t);
    bool lz = false, rz = false, fh = false, ignRate = false;
    RecordHeadFlags fl;
   #if OGI_TEST_HOOKS
    lz = hooks.lambdaRampZero;
    rz = hooks.runRampZero;
    fh = hooks.freezeHardStop;
    ignRate = hooks.runIgnoreRate;
    fl.settleOff = hooks.settleOff;
   #endif

    if (lz)
        fbRamp.setCell (gp.feedback, gp.feedback);
    else
    {
        fbRamp.setTarget (gp.feedback, rampCells20);
        fbRamp.tick();
    }
    {
        // PLAN-2.5a PD6: routed feedback modulates the λ ramp in its domain (0..1).
        const bool rk = matrix.routed (GiDest::Feedback);
        const float k = rk ? GiModMap::pctFromRaw (matrix.apply (GiDest::Feedback, fbRamp.current() * 100.0f, matrix.offset (GiDest::Feedback)))
                           : fbRamp.current();
        fbMod.setCell (fbNow, k);
        fbNow = k;
        useFbMod = rk || matrix.routedPrev (GiDest::Feedback);
        traceAt (GiDest::Feedback, k);
    }

    const bool freezeEdge = gp.freeze && ! prevFreeze;
    prevFreeze = gp.freeze;

    bool anySolo = false;
    for (int s = 0; s < kGiStreams; ++s)
        anySolo = anySolo || (gp.s[s].enable && gp.s[s].solo);
    const bool anyVoice = VoiceAllocator::countCounted (voices) > 0;

    for (int s = 0; s < kTapeSlots; ++s)
    {
        auto& hd = head[s];
        const SlotTape* tp = current[(size_t) s];

        // Held return peak of the previous cell (50 ms release at the tick).
        float pk = 0.0f;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < kFbDelay; ++i)
            {
                const float a = std::abs (fbRet[s][ch][i]);
                if (a > pk) pk = a;   // NaN never wins
            }
        hd.returnPk = std::max (pk, hd.returnPk * pkRel);

        const bool pending = handoff.pendingDepth (s) > 0;
        hd.swapWaiting = pending && ! hd.parked;
        const bool rateOk = tp != nullptr && (ignRate || kaiser::ratesEqual (tp->rate, fs));
        const bool voiceTerm = gp.s[s].enable && audibleTarget (s, anySolo) && anyVoice;
        const bool target = ! gp.freeze && rateOk && ! xfade[s].active && ! pending
                         && (voiceTerm || hd.returnPk > 1.0e-5f);
        const float v = target ? 1.0f : 0.0f;

        if (fh && freezeEdge)
            hd.run.reset (0.0f);
        if (rz)
            hd.run.setCell (v, v);
        else
        {
            hd.run.setTarget (v, rampCells20);
            hd.run.tick();
        }

        const bool parkedNow = fbEff().settledAt (0.0f) || hd.run.settledAt (0.0f);
        if (parkedNow && ! hd.parked && tp != nullptr)
            hd.park (current[(size_t) s]->tape, fl, headCounters (s));
        hd.parked = parkedNow;
    }
}

/** PLAN-2.4 PD3: every running head writes its whole cell at the tick. */
void Engine::writeHeads (std::int64_t t) noexcept
{
    RecordHeadFlags fl;
   #if OGI_TEST_HOOKS
    fl.rClampOff     = hooks.rClampOff;
    fl.writeScrubOff = hooks.writeScrubOff;
    fl.settleOff     = hooks.settleOff;
   #else
    juce::ignoreUnused (t);
   #endif
    for (int s = 0; s < kTapeSlots; ++s)
    {
        auto& hd = head[s];
        SlotTape* tp = current[(size_t) s];
        if (hd.parked || tp == nullptr)
        {
            hd.cellStart = hd.h;
            hd.wrapOff = -1;
            continue;
        }
        const float* rL = fbRet[s][0];
        const float* rR = fbRet[s][1];
       #if OGI_TEST_HOOKS
        float rInj[2][kFbDelay];
        auto& inj = hooks.injectReturn;
        if (inj.slot == s && ! inj.done && inj.atSample >= t && inj.atSample < t + kCell)
        {
            std::memcpy (rInj, fbRet[s], sizeof (rInj));
            const int i = static_cast<int> (inj.atSample - t);
            rInj[0][i] = rInj[1][i] = inj.value;
            inj.done = true;
            rL = rInj[0];
            rR = rInj[1];
        }
       #endif
        auto& c = headCounters (s);
        hd.writeCell (tp->tape, rL, rR, fbEff(),
                      region[s].lo, region[s].hi, tp->frames, fl, c);
        foldOverview (s);   // PLAN-2.6 T11: the evolving overview
    }
}

/** PLAN-2.4 PD11: the reverb's engine-owned atomics, written at grid ticks
    only (parent Engine.cpp:222-235 without the 2.5 matrix lines). Fixed
    internals (enabled 1, predelay 20 ms, mod 0.2, shimmer 0) are set once. */
void Engine::writeReverbAtomics() noexcept
{
    // PLAN-2.5a PD6: tick-rate atomics; routed → the modulated raw / 100
    // (parent :222-235). R54: mix <= 0.001 restarts a cold tank (INFO).
    using namespace GiDest;
    float size = gp.reverbSize, damp = gp.reverbDamp, mixV = gp.reverbMix;
    if (matrix.routed (RevSize)) size = matrix.apply (RevSize, matrix.rawNow (RevSize), matrix.offset (RevSize)) / 100.0f;
    if (matrix.routed (RevDamp)) damp = matrix.apply (RevDamp, matrix.rawNow (RevDamp), matrix.offset (RevDamp)) / 100.0f;
    if (matrix.routed (RevMix))  mixV = matrix.apply (RevMix,  matrix.rawNow (RevMix),  matrix.offset (RevMix))  / 100.0f;
    revSize.store (size, std::memory_order_relaxed);
    revDamp.store (damp, std::memory_order_relaxed);
    revMix.store (mixV, std::memory_order_relaxed);
    traceAt (RevSize, size);
    traceAt (RevDamp, damp);
    traceAt (RevMix, mixV);
}

void Engine::mixPiece (int c0, int c1, float* outL, float* outR, std::int64_t tAbs) noexcept
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
        // PLAN-2.4 PD8: the stream's tap accumulators start at 0 every piece.
        std::fill (tapAcc[s][0] + c0, tapAcc[s][0] + c1, 0.0f);
        std::fill (tapAcc[s][1] + c0, tapAcc[s][1] + c1, 0.0f);
        std::fill (envSum[s] + c0, envSum[s] + c1, 0.0f);
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
            const CellLerp& gf = gFbLerp[k][s];
            float* tL = tapAcc[s][0];
            float* tR = tapAcc[s][1];
            float* es = envSum[s];
            if (pairMix[s])
            {
                // PLAN-2.5a PD7: the pair mixer mode (per-voice level / pan).
                const CellLerp& pl = pairGL[k][s];
                const CellLerp& pr = pairGR[k][s];
                const CellLerp& pv = pairLv[k][s];
                for (int c = c0; c < c1; ++c)
                {
                    const float a = envBuf[k][c] * lg.at (c);
                    if (a > 0.0f)
                    {
                        aL[c] += a * (bl[c] * pl.at (c));
                        aR[c] += a * (br[c] * pr.at (c));
                    }
                    const float e = envBuf[k][c];
                    if (e > 0.0f)
                    {
                        const float g = e * gf.at (c) * std::min (pv.at (c), 1.0f);
                        tL[c] += g * bl[c];
                        tR[c] += g * br[c];
                        es[c] += e;
                    }
                    bl[c] = 0.0f;
                    br[c] = 0.0f;
                }
                continue;
            }
            for (int c = c0; c < c1; ++c)
            {
                const float a = envBuf[k][c] * lg.at (c);
                if (a > 0.0f)
                {
                    aL[c] += a * bl[c];
                    aR[c] += a * br[c];
                }
                // PLAN-2.4 PD8 / P11: the feedback tap, separate statements
                // (the mix statements above are untouched): Σ_k a_k·g_fb·bus.
                const float e = envBuf[k][c];
                if (e > 0.0f)
                {
                    const float g = e * gf.at (c);
                    tL[c] += g * bl[c];
                    tR[c] += g * br[c];
                    es[c] += e;
                }
                bl[c] = 0.0f;
                br[c] = 0.0f;
            }
        }
    }

    // PLAN-2.4 PD7 / PD8: tap → 4 chains → fbRet (every piece, frozen or not).
    runChains (c0, c1, tAbs);

    const float vgain = voiceGainNow;
    const int len = c1 - c0;
    for (int c = c0; c < c1; ++c)
    {
        // Per stream level / audibility / pan, voice headroom (PD12 statements,
        // unchanged; PLAN-2.4 PD11 step 1 stores y into the wet bus).
        float yL = 0.0f, yR = 0.0f;
        bool have = false;
        for (int s = 0; s < kGiStreams; ++s)
        {
            if (! audible[s])
                continue;
            float tL = 0.0f, tR = 0.0f;
            if (pairMix[s])
            {
                const float lv = audibleRamp[s].value (c) * vgain;   // PD7: level · pan per pair
                tL = accL[s][c] * lv;
                tR = accR[s][c] * lv;
            }
            else
            {
                const float lv = levelR (s).value (c) * audibleRamp[s].value (c) * vgain;
                tL = accL[s][c] * lv * panLR (s).value (c);
                tR = accR[s][c] * lv * panRR (s).value (c);
            }
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
            if (pairMix[0])
            {
                const float lv = audibleRamp[0].value (c) * vgain;
                yL = accL[0][c] * lv;
                yR = accR[0][c] * lv;
            }
            else
            {
                const float lv = levelR (0).value (c) * audibleRamp[0].value (c) * vgain;
                yL = accL[0][c] * lv * panLR (0).value (c);
                yR = accR[0][c] * lv * panRR (0).value (c);
            }
        }

        wet[0][c - c0] = yL;
        wet[1][c - c0] = yR;
    }

    // 2. R96 wet guard (parent Engine.cpp:1612-1634): a non-finite wet sample
    //    never enters the reverb tank (a module state the engine cannot scrub).
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < len; ++i)
            if (! std::isfinite (wet[ch][i]))
            {
                wet[ch][i] = 0.0f;
               #if OGI_TEST_HOOKS
                ++hooks.wetScrubs;
               #endif
            }

    // 3. Reverb over the piece, in place (mix 0 and not running: the unit
    //    returns before touching the buffer — a bitwise bypass).
    {
        float* const chans[2] { wet[0], wet[1] };
        revView.setDataToReferTo (chans, 2, len);
        reverb.process (revView);
    }

   #if OGI_TEST_HOOKS
    {
        auto& inj = hooks.injectPreKnee;
        if (inj.atSample >= 0 && ! inj.done && tAbs + len > inj.atSample && tAbs <= inj.atSample)
        {
            const int i = static_cast<int> (inj.atSample - tAbs);
            wet[0][i] = wet[1][i] = inj.value;
            inj.done = true;
        }
    }
   #else
    juce::ignoreUnused (tAbs);
   #endif

    // 4. Wet safety = the parent D9 knee (PLAN-2.4 PD1; identity at or below
    //    kKneeT), then 5. output_gain as its own statement.
    for (int c = c0; c < c1; ++c)
    {
        float yL = wet[0][c - c0];
        float yR = wet[1][c - c0];
       #if OGI_TEST_HOOKS
        hooks.preKneePeak = std::max ({ hooks.preKneePeak, std::abs (yL), std::abs (yR) });
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
       #if OGI_TEST_HOOKS
        hooks.kneePeak = std::max ({ hooks.kneePeak, std::abs (yL), std::abs (yR) });
       #endif
        const float og = (useOutMod ? outGainMod : outGainRamp).value (c);
       #if OGI_TEST_HOOKS
        if (hooks.scNoClear)   // LK1n: an accumulating mixer over an uncleared buffer
        {
            outL[c - c0] += yL * og;
            outR[c - c0] += yR * og;
            continue;
        }
       #endif
        outL[c - c0] = yL * og;
        outR[c - c0] = yR * og;
    }
}

//==============================================================================
void Engine::bypassedBlock() noexcept
{
    recorder.bypassedBlock();   // PD10: Armed → cancel (Bypassed); Recording → stop
    serviceTapeSnapshot();      // PLAN-3.2 PD18: a Save still completes while bypassed
}
} // namespace ogi
