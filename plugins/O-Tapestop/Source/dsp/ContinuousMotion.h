/*
   This file is part of O-Tapestop, an Ouaricon Audio plugin.
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

    O-Tapestop - ContinuousMotion (v1.1 Continuous mode)

    Per-sample rate generator for MODE = Continuous: while ENGAGE is latched,
    the carrier's rate r moves continuously instead of running one gesture.
    Three characters (research/o-tapestop-continuous-mode.md):

      Wobble  — deterministic wow sine + ChowTapeModel 3-harmonic flutter
                stack (0.56/0.20/0.24 of 230:80:99); CHAOS morphs order →
                disorder (per-cycle freq/amp jitter, OU drift, filtered-noise
                band above 0.6).
      Random  — Ornstein-Uhlenbeck octave stack (τ, τ/4, τ/16 — weights
                1, 0.5·chaos, 0.25·chaos²), tanh excursion shaping, 20 Hz
                post-LP so r stays C1 ("digital zipper" guard).
      Glitch  — grid scheduler: monophonic events, one Bernoulli draw per
                slot (p = chaos²; 1→5 slots per cell unlock as chaos rises
                past 0.4, slot starts jitter off the grid above 0.5, and
                event lengths shorten with chaos down to ⅛-cell micro-
                bursts, so high chaos is a rapid-fire barrage instead of
                one texture per cell). Tame family from chaos 0 (tapestop-dip, half-speed
                drag, speed-jump, stutter-repeat); extreme family unlocks
                above 0.5 (reverse-flick, dead-stop freeze, +2-rail slam,
                square-wave chatter, buffer-shuffle back-jump, stutter
                halving-roll / pitch-ramp variants, resync-snap).

    CONTINUITY INVARIANT: r may step; position may not. Rate discontinuities
    are C1 corners (click-free — the ScratchPass palindrome-corner precedent),
    softened by a 2 ms slew; position jumps (stutter loop-back, resync-snap)
    are ONLY ever requested via Out::jumpKind so the transport splices them
    with the 2-voice crossfade.

    DEBT SAFETY (26 s ring): zero-mean wobble is bounded by construction;
    Random's position debt random-walks even though its rate mean-reverts, so
    a weak servo (kServo = 0.2 /s, contribution clamped to ±0.2 % ≈ 3.5 cents)
    recenters debt on the engage-latched target. Glitch events accumulate
    one-sided debt: selection weights carry exp(−λ·debt·sign) bias, debt-
    positive events are suppressed above kSoftDebtSeconds, and above
    kHardDebtSeconds a resync-snap splice is forced at the next boundary —
    the safety mechanism doubles as an event type. The shuffle back-jump
    adds position debt directly, so its amount is capped at 2 s AND the
    remaining hard-budget headroom.

    DETERMINISM: three dedicated xorshift64* streams (noise / event-timing /
    event-pick — one stream per purpose, never shared), re-seeded from fixed
    constants at every engage edge. All state advances per sample inside
    tick(), so output is a pure function of samples-since-engage: P0
    determinism and 512-vs-4096 bit-identity hold by construction.

    RT-safe: no allocation, no locks; everything latched in engage() or set
    via setTargets() (plain doubles written from the audio thread's own
    16-sample grid).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include <cmath>
#include <cstdint>

class ContinuousMotion
{
public:
    enum class Character { Wobble = 0, Random = 1, Glitch = 2 };

    struct Params
    {
        Character character       = Character::Wobble;
        double    mPeak           = 0.004;  // peak fractional speed deviation
        double    depth01         = 0.5;    // raw DEPTH 0..1 (Glitch intensity)
        double    chaos           = 0.0;    // 0..1
        double    periodSamples   = 48000;  // wobble LFO period / glitch cell
        double    debtNowSamples  = 0.0;    // carrier debt at the engage edge
    };

    struct Out
    {
        double r          = 1.0;
        int    jumpKind   = 0;    // 0 none, 1 loop-back by jumpAmount, 2 snap to (live − jumpAmount)
        double jumpAmount = 0.0;  // samples
        int    xfLen      = 0;    // splice length in samples (jumpKind != 0 only)
    };

    // Servo / debt policy (research doc "Debt management").
    static constexpr double kServoPerSecond   = 0.2;
    static constexpr double kServoClamp       = 0.002;  // ±0.2 % ≈ ±3.5 cents
    static constexpr double kSoftDebtSeconds  = 3.0;
    static constexpr double kHardDebtSeconds  = 6.0;    // ≪ 26 s ring
    static constexpr double kSlewSeconds      = 0.002;  // glitch r-step softening
    static constexpr double kDepthSlewSeconds = 0.05;   // ChowTape 50 ms precedent

    void prepare (double sampleRate) noexcept
    {
        fs = sampleRate;
        depthSlewCoeff = std::exp (-1.0 / (kDepthSlewSeconds * fs));
        noiseLpCoeff   = std::exp (-2.0 * juce::MathConstants<double>::pi * 20.0 / fs);
        bandLpCoeff    = std::exp (-2.0 * juce::MathConstants<double>::pi * 30.0 / fs);
        driftCoeff     = std::exp (-1.0 / (5.0 * fs));          // τ = 5 s wow drift
        slewStep       = 2.0 / (kSlewSeconds * fs);             // full ±1→∓1 swing in 2 ms
        reset();
    }

    void reset() noexcept
    {
        character = Character::Wobble;
        mPeakCur = mPeakTarget = 0.0;
        depth01 = chaos = 0.0;
        periodCur = periodTarget = 48000.0;
        debtTarget = 0.0;
        phaseW = phaseF = 0.0;
        wowJitter = 0.0;
        wowAmpJitter = 1.0;
        ou1 = ou2 = ou3 = ouDrift = bandState = mLp = 0.0;
        rCur = 1.0;
        cellLen = 1;
        cellPos = 0;
        slotLen = 1;
        slotWait = 0;
        slotArmed = false;
        eventActive = false;
        eventType = 0;
        eventPos = eventLen = 0;
        sliceLen = slicePos = 0;
        sliceXf = 0;
        stutterRate = stutterRateMult = 1.0;
        stutterRoll = false;
        chatterHalf = 1;
        chatterAmp = 0.0;
        shuffleAmount = 0.0;
        shuffleXf = 0;
        shuffleEmitted = false;

        // Jump-guard state. Covered here as well as in engage() so reset() is
        // the FULL state set — a re-prepare must not leave a stale fade length
        // or emitted flag behind for the next engage to trip over.
        samplesSinceJump = 1 << 30;
        lastXfLen = 0;
        snapEmitted = false;
    }

    /** ENGAGE edge (audio thread). Latches character/targets and RE-SEEDS all
        three RNG streams from fixed constants — every engage replays the same
        stochastic sequence (repeatable bounces; P0 two-instance bitwise). */
    void engage (const Params& p) noexcept
    {
        character    = p.character;
        depth01      = p.depth01;
        chaos        = p.chaos;
        mPeakTarget  = p.mPeak;
        mPeakCur     = p.mPeak;          // no slew-in from 0 — depth is latched live afterwards
        periodTarget = juce::jmax (2.0, p.periodSamples);
        periodCur    = periodTarget;
        recomputeOuCoeffs();

        // Debt target: engage-edge debt, floored at 50 ms so wobble/random
        // r > 1 half-cycles have headroom instead of clipping at the live head
        // forever (the FIRST upswing from a Bypassed engage still clips —
        // accepted; the servo builds headroom within seconds).
        debtTarget = juce::jmax (p.debtNowSamples, 0.05 * fs);

        noiseRng = kSeedNoise;
        timeRng  = kSeedTime;
        pickRng  = kSeedPick;

        phaseW = phaseF = 0.0;
        wowJitter = 0.0;
        wowAmpJitter = 1.0;
        ou1 = ou2 = ou3 = ouDrift = bandState = mLp = 0.0;
        rCur = 1.0;

        cellLen = (int) juce::jmax (2.0, periodCur);
        cellPos = 0;
        recomputeSlots();
        slotWait = 0;
        slotArmed = false;
        eventActive = false;
        samplesSinceJump = 1 << 30;
    }

    /** Live target updates (audio thread, absolute 16-sample grid — the
        toneTrack cadence). Depth slews over 50 ms; period is consumed at the
        next LFO wrap / cell boundary (per-cycle latch, so a mid-cycle tempo
        or rate move never bends a running cycle). */
    void setTargets (double mPeakNew, double depth01New, double periodNew) noexcept
    {
        mPeakTarget  = mPeakNew;
        depth01      = depth01New;
        periodTarget = juce::jmax (2.0, periodNew);
        if (character == Character::Random)
            recomputeOuCoeffs();
    }

    /** One sample of continuous motion. `debtSamples` = live − carrier pos. */
    Out tick (double debtSamples) noexcept
    {
        Out out;
        ++samplesSinceJump;

        // 50 ms one-pole depth slew (every sample, every character).
        mPeakCur = mPeakTarget + (mPeakCur - mPeakTarget) * depthSlewCoeff;

        switch (character)
        {
            case Character::Wobble: out.r = tickWobble (debtSamples); break;
            case Character::Random: out.r = tickRandom (debtSamples); break;
            case Character::Glitch: tickGlitch (debtSamples, out);    break;
        }

        // Hard rail: the engine's historical rate bound (scratch precedent).
        out.r = juce::jlimit (-2.0, 2.0, out.r);
        rCur  = out.r;
        return out;
    }

private:
    // ── shared helpers ──────────────────────────────────────────────────────

    // xorshift64* — three independent streams, one per purpose
    // (pattern_rng_stream_interleave_blocksize).
    static constexpr std::uint64_t kSeedNoise = 0x9E3779B97F4A7C15ull;
    static constexpr std::uint64_t kSeedTime  = 0xBF58476D1CE4E5B9ull;
    static constexpr std::uint64_t kSeedPick  = 0x94D049BB133111EBull;

    static double nextUniform (std::uint64_t& s) noexcept   // [0, 1)
    {
        s ^= s >> 12;
        s ^= s << 25;
        s ^= s >> 27;
        return (double) ((s * 0x2545F4914F6CDD1Dull) >> 11)
             * (1.0 / 9007199254740992.0);
    }

    static double nextBipolar (std::uint64_t& s) noexcept   // (−1, 1)
    {
        return 2.0 * nextUniform (s) - 1.0;
    }

    // Unit-variance excitation: uniform on (−√3, √3). The exact distribution
    // is irrelevant after the OU/LP filtering; uniform avoids Box-Muller's
    // log/sqrt per sample.
    static double nextUnitVar (std::uint64_t& s) noexcept
    {
        return 1.7320508075688772 * nextBipolar (s);
    }

    void recomputeOuCoeffs() noexcept
    {
        // τ = period/(2π) samples; octave stack at τ, τ/4, τ/16.
        const double tau = juce::jmax (2.0, periodTarget / juce::MathConstants<double>::twoPi);
        a1 = std::exp (-1.0 / tau);
        a2 = std::exp (-4.0 / tau);
        a3 = std::exp (-16.0 / tau);
        e1 = std::sqrt (juce::jmax (0.0, 1.0 - a1 * a1));
        e2 = std::sqrt (juce::jmax (0.0, 1.0 - a2 * a2));
        e3 = std::sqrt (juce::jmax (0.0, 1.0 - a3 * a3));
    }

    double servo (double debtSamples) const noexcept
    {
        return juce::jlimit (-kServoClamp, kServoClamp,
                             kServoPerSecond * (debtTarget - debtSamples) / fs);
    }

    // ── Wobble ──────────────────────────────────────────────────────────────
    double tickWobble (double debtSamples) noexcept
    {
        // Per-cycle latch: frequency (and its chaos jitter) changes only at
        // the wrap — phase-continuous by construction (ChowTape wow drift).
        phaseW += (1.0 + wowJitter) / periodCur;
        if (phaseW >= 1.0)
        {
            phaseW -= 1.0;
            periodCur    = periodTarget;
            wowJitter    = 0.5 * chaos * std::pow (nextUniform (timeRng), 1.25);
            wowAmpJitter = juce::jmax (0.1, 1.0 + 0.6 * chaos * nextBipolar (timeRng));
        }

        // Flutter stack rides at 6×wow, clamped to the 6–40 Hz flutter band
        // (research: motor/pulley harmonics at 1×/2×/3× — 230:80:99 → 0.56/0.20/0.24).
        const double fWow  = fs / periodCur;
        const double fFlut = juce::jlimit (6.0, 40.0, 6.0 * fWow);
        phaseF += fFlut / fs;
        if (phaseF >= 1.0)
            phaseF -= 1.0;

        const double pw = juce::MathConstants<double>::twoPi * phaseW;
        const double pf = juce::MathConstants<double>::twoPi * phaseF;

        double m = mPeakCur * (0.8 * wowAmpJitter * std::sin (pw)
                               + 0.2 * (0.56 * std::sin (pf)
                                        + 0.20 * std::sin (2.0 * pf)
                                        + 0.24 * std::sin (3.0 * pf)));

        // Chaos: slow OU drift (τ = 5 s) + filtered-noise flutter band above 0.6.
        ouDrift = driftCoeff * ouDrift
                + std::sqrt (juce::jmax (0.0, 1.0 - driftCoeff * driftCoeff)) * nextUnitVar (noiseRng);
        m += (0.5 * chaos * mPeakCur) * ouDrift;

        if (chaos > 0.6)
        {
            bandState = bandLpCoeff * bandState + (1.0 - bandLpCoeff) * nextBipolar (noiseRng);
            m += (0.5 * (chaos - 0.6) / 0.4) * mPeakCur * bandState * 10.0; // LP(30 Hz) gain makeup
        }

        return 1.0 + juce::jlimit (-2.0 * mPeakCur, 2.0 * mPeakCur, m) + servo (debtSamples);
    }

    // ── Random ──────────────────────────────────────────────────────────────
    double tickRandom (double debtSamples) noexcept
    {
        ou1 = a1 * ou1 + e1 * nextUnitVar (noiseRng);
        ou2 = a2 * ou2 + e2 * nextUnitVar (noiseRng);
        ou3 = a3 * ou3 + e3 * nextUnitVar (noiseRng);

        const double s = ou1 + 0.5 * chaos * ou2 + 0.25 * chaos * chaos * ou3;
        const double m = mPeakCur * std::tanh ((0.5 + chaos) * s);

        // 20 Hz post-LP: raw OU is nowhere-differentiable — this keeps r C1.
        mLp = noiseLpCoeff * mLp + (1.0 - noiseLpCoeff) * m;

        return 1.0 + mLp + servo (debtSamples);
    }

    // ── Glitch ──────────────────────────────────────────────────────────────
    // Event types (Δdebt sign drives the selection bias):
    enum EventType { evDip = 0, evHalf, evJumpUp, evReverse, evStutter, evSnap,
                     evFreeze, evSlam, evChatter, evShuffle, evCount };

    void tickGlitch (double debtSamples, Out& out) noexcept
    {
        const double debtSec = debtSamples / fs;

        // Slot-start jitter (chaos > 0.5): defer each slot's attempt by up to
        // 35 % of a slot so events fall off the grid — rigid slot timing reads
        // as rhythmic, not erratic. g = 0 draws nothing, so the low-chaos RNG
        // stream (and the ≤ 0.4 v1.1 cadence) is bit-identical to v1.2.
        if (! eventActive && cellPos % slotLen == 0)
        {
            const double g = juce::jmax (0.0, chaos - 0.5) * 2.0;
            slotWait  = g > 0.0
                          ? (int) (nextUniform (timeRng) * 0.35 * g * (double) slotLen)
                          : 0;
            slotArmed = true;
        }
        if (slotArmed && ! eventActive && --slotWait < 0)
        {
            slotArmed = false;
            maybeStartEvent (debtSec);
        }

        double rTarget = 1.0;

        if (eventActive)
        {
            const double u = (double) eventPos / (double) juce::jmax (1, eventLen);

            switch (eventType)
            {
                case evDip:
                {
                    // Tapestop dip: down x² to the floor, back up x² (the
                    // Stop-mode curve law at its physics default p = 2).
                    const double floorR = juce::jmax (0.0, 1.0 - depth01);
                    const double shape  = u < 0.5 ? (1.0 - 2.0 * u) * (1.0 - 2.0 * u)
                                                  : (2.0 * u - 1.0) * (2.0 * u - 1.0);
                    rTarget = floorR + (1.0 - floorR) * shape;
                    break;
                }

                case evHalf:    rTarget = 1.0 - 0.5 * depth01;  break;
                case evJumpUp:  rTarget = 1.0 + depth01;        break;   // ≤ 2 by depth ≤ 1

                case evReverse:   // chaos boosts the flick toward the −2 rail
                    rTarget = -juce::jmin (2.0, depth01 * (1.0 + 2.0 * juce::jmax (0.0, chaos - 0.5)));
                    break;

                case evFreeze:  rTarget = 0.0;  break;   // dead stop, instant resume
                case evSlam:    rTarget = 2.0;  break;   // hold the engine rail

                case evChatter:  // square-wave 1 ± depth; the 2 ms slew keeps edges C1
                    rTarget = ((eventPos / chatterHalf) & 1) != 0 ? 1.0 - chatterAmp
                                                                  : 1.0 + chatterAmp;
                    break;

                case evShuffle:
                {
                    // Single back-jump through the splice path (same previous-
                    // fade guard as stutter/snap); replays recent audio.
                    rTarget = 1.0;
                    if (! shuffleEmitted && samplesSinceJump >= lastXfLen)
                    {
                        out.jumpKind     = 1;             // loop back
                        out.jumpAmount   = shuffleAmount;
                        out.xfLen        = shuffleXf;
                        lastXfLen        = shuffleXf;
                        shuffleEmitted   = true;
                        samplesSinceJump = 0;
                    }
                    break;
                }

                case evStutter:
                {
                    // The jump waits for the PREVIOUS fade (whatever kind) to
                    // fully complete — a force-complete inside startXfade
                    // drops a voice at its residual gain, and a fade only a
                    // few samples in still carries ~all of the signal (an
                    // audible amplitude step, caught by C-P6). slicePos keeps
                    // counting past sliceLen, so a deferred jump fires on the
                    // first clean sample.
                    rTarget = stutterRate;
                    if (++slicePos >= sliceLen && samplesSinceJump >= lastXfLen)
                    {
                        slicePos       = 0;
                        out.jumpKind   = 1;                 // loop back one slice
                        out.jumpAmount = (double) sliceLen;
                        out.xfLen      = sliceXf;
                        lastXfLen      = sliceXf;
                        samplesSinceJump = 0;

                        // Variants: halving roll shrinks the slice each repeat
                        // (stops while ≥ 2 fades fit); pitch ramp walks the
                        // repeat rate ±2 semitones per loop-back.
                        if (stutterRoll)
                        {
                            const int minSlice = 2 * (int) std::lround (0.003 * fs);
                            if (sliceLen / 2 >= minSlice)
                            {
                                sliceLen /= 2;
                                sliceXf   = (int) juce::jlimit (0.003 * fs, 0.05 * fs,
                                                                (double) sliceLen / 4.0);
                            }
                        }
                        stutterRate = juce::jlimit (0.25, 2.0, stutterRate * stutterRateMult);
                    }
                    break;
                }

                case evSnap:
                {
                    // Same previous-fade guard as the stutter jump: a snap
                    // drawn on the cell boundary right after a stutter's last
                    // loop-back must not force-complete that fade.
                    rTarget = 1.0;
                    if (! snapEmitted && samplesSinceJump >= lastXfLen)
                    {
                        const int xf   = (int) std::lround (0.05 * fs);
                        out.jumpKind   = 2;                 // snap to live − target debt
                        out.jumpAmount = debtTarget;
                        out.xfLen      = xf;
                        lastXfLen      = xf;
                        snapEmitted    = true;
                        samplesSinceJump = 0;
                    }
                    break;
                }

                default: break;
            }

            if (++eventPos >= eventLen)
                eventActive = false;
        }

        if (++cellPos >= cellLen)
        {
            cellPos = 0;
            cellLen = (int) juce::jmax (2.0, periodTarget);   // per-cell latch
            recomputeSlots();
        }

        // 2 ms rate slew: C1 corners → C2 (faint-tick guard on bright
        // material); perceptually still an instant switch.
        const double d = rTarget - rCur;
        double r = (std::abs (d) <= slewStep) ? rTarget
                                              : rCur + (d > 0.0 ? slewStep : -slewStep);

        // Between events only, the weak servo recenters leftover debt.
        if (! eventActive)
            r += servo (debtSamples);

        out.r = r;
    }

    void recomputeSlots() noexcept
    {
        // 1 slot ≤ chaos 0.4 (v1.1 cadence), then 2..5 as chaos rises —
        // event DENSITY, not just event character, scales with chaos.
        const int slotCount = 1 + (int) std::floor (4.0 * juce::jmax (0.0, chaos - 0.4) / 0.6);
        slotLen = juce::jmax (1, cellLen / slotCount);
    }

    void maybeStartEvent (double debtSec) noexcept
    {
        // Hard budget: force the resync-snap regardless of the draw.
        if (debtSec > kHardDebtSeconds)
        {
            startEvent (evSnap, debtSec);
            return;
        }

        if (nextUniform (timeRng) >= chaos * chaos)   // p(event) = chaos² per slot
            return;

        // Weights: tame events from chaos 0 (fading 50 % as chaos maxes);
        // extremes unlock above 0.5 and ramp hard. Δdebt sign s: dip/half/
        // reverse/stutter/freeze/shuffle +1, jumpUp/slam/snap −1, chatter
        // rate-neutral; bias w·exp(−λ·debt·s) self-centres net drift
        // (λ = 0.5 /s).
        const double g    = juce::jmax (0.0, chaos - 0.5) * 2.0;
        const double lam  = 0.5;
        const double bPos = std::exp (-lam * debtSec);   // debt-positive events
        const double bNeg = std::exp ( lam * debtSec);   // debt-negative events
        const bool   soft = debtSec > kSoftDebtSeconds;

        double w[evCount];
        w[evDip]     = (soft ? 0.0 : 1.0 - 0.65 * g) * bPos;
        w[evHalf]    = (soft ? 0.0 : 0.8 * (1.0 - 0.65 * g)) * bPos;
        w[evJumpUp]  = 0.6 * bNeg;
        w[evReverse] = (soft ? 0.0 : 0.7 * g) * bPos;
        w[evStutter] = (soft ? 0.0 : 1.0 + 0.5 * g) * bPos;
        w[evSnap]    = 0.3 * g * bNeg;
        w[evFreeze]  = (soft ? 0.0 : 0.9 * g) * bPos;
        w[evSlam]    = 0.8 * g * bNeg;
        w[evChatter] = (soft ? 0.0 : 0.8 * g);           // neutral: no debt bias
        w[evShuffle] = (soft ? 0.0 : 0.9 * g) * bPos;

        double total = 0.0;
        for (double wi : w)
            total += wi;
        if (total <= 0.0)
            return;

        double pick = nextUniform (pickRng) * total;
        int    type = evCount - 1;
        for (int i = 0; i < evCount; ++i)
        {
            pick -= w[i];
            if (pick < 0.0) { type = i; break; }
        }

        // Low-depth retargets: an extreme drawn at low depth would be a
        // barely-audible crawl — swap to the same-debt-sign tame cousin
        // (the v1.1 reverse→dip precedent).
        if (type == evReverse && depth01 < 0.4)  type = evDip;
        if (type == evFreeze  && depth01 < 0.3)  type = evHalf;
        if (type == evSlam    && depth01 < 0.3)  type = evJumpUp;
        if (type == evChatter && depth01 < 0.25) type = evHalf;

        startEvent (type, debtSec);
    }

    void startEvent (int type, double debtSec) noexcept
    {
        eventActive = true;
        eventType   = type;
        eventPos    = 0;
        eventLen    = juce::jmax (2, cellLen);
        snapEmitted = false;

        const double g = juce::jmax (0.0, chaos - 0.5) * 2.0;

        // Event length: full cell at chaos 0 (v1.1 behavior); ½ and ¼
        // fractions unlock with chaos, plus an ⅛ micro-burst tier above 0.5,
        // so high-chaos events land as bursts. Snap keeps the full cell (it
        // only waits for the fade guard). All g-terms vanish at chaos ≤ 0.5,
        // keeping that region bit-identical to v1.2.
        if (type != evSnap)
        {
            const double wLong  = 1.0;
            const double wMid   = 1.5 * chaos;
            const double wShort = (2.2 + 1.6 * g) * chaos * chaos;
            const double wMicro = 1.4 * g * chaos * chaos;
            double u = nextUniform (timeRng) * (wLong + wMid + wShort + wMicro);
            const double frac = u < wLong                  ? 1.0
                              : u < wLong + wMid           ? 0.5
                              : u < wLong + wMid + wShort  ? 0.25 : 0.125;
            eventLen = juce::jmax (2, (int) ((double) cellLen * frac));
        }

        if (type == evStutter)
        {
            // Slice = eventLen/2^k (k = 1..3 — deeper cuts read as micro-
            // buzz), spliced at clamp(slice/4, 3 ms, 50 ms); slice is kept
            // ≥ 2 fades so a force-complete never carries real residual
            // gain (the 2-voice pool stays structural). Chaos-gated
            // variants: halving roll, ±2-semitone-per-repeat pitch ramp.
            const int k = 1 + (int) (nextUniform (pickRng) * 3.0);
            const int slice = juce::jmax (2, eventLen >> k);
            sliceXf  = (int) juce::jlimit (0.003 * fs, 0.05 * fs, (double) slice / 4.0);
            sliceLen = juce::jmax (2 * sliceXf, slice);
            if (eventLen < 2 * sliceLen)
                eventLen = 2 * sliceLen;
            slicePos = 0;

            const double uv = nextUniform (pickRng) * (1.0 + 1.8 * g);
            stutterRoll     = uv >= 1.0 && uv < 1.0 + 0.9 * g;
            const bool ramp = uv >= 1.0 + 0.9 * g;
            stutterRate     = 1.0;
            stutterRateMult = ! ramp ? 1.0
                                     : (nextUniform (pickRng) < 0.5 ? 1.12246 : 1.0 / 1.12246);
        }
        else if (type == evChatter)
        {
            const double f = 10.0 + 20.0 * nextUniform (timeRng);   // 10–30 Hz
            chatterHalf = juce::jmax (1, (int) std::lround (fs / (2.0 * f)));
            chatterAmp  = juce::jlimit (0.0, 1.0, depth01);
        }
        else if (type == evShuffle)
        {
            // Back-jump 1–4 cells, capped at 2 s AND the remaining hard-
            // budget headroom (a shuffle must never jump the debt past the
            // snap rail). Too little headroom degrades to a plain hold.
            const int m = 1 + (int) (nextUniform (timeRng) * 4.0);
            const double headroom = juce::jmax (0.0, (kHardDebtSeconds - debtSec) * fs * 0.9);
            shuffleAmount  = juce::jmin ((double) m * cellLen, 2.0 * fs, headroom);
            shuffleXf      = (int) juce::jlimit (0.003 * fs, 0.05 * fs, (double) cellLen / 16.0);
            shuffleEmitted = shuffleAmount < 2.0 * shuffleXf;
        }
    }

    // ── state ───────────────────────────────────────────────────────────────
    double fs = 48000.0;

    Character character = Character::Wobble;
    double mPeakCur = 0.0, mPeakTarget = 0.0;
    double depth01 = 0.0, chaos = 0.0;
    double periodCur = 48000.0, periodTarget = 48000.0;
    double debtTarget = 0.0;

    double depthSlewCoeff = 0.0, noiseLpCoeff = 0.0, bandLpCoeff = 0.0, driftCoeff = 0.0;
    double slewStep = 1.0;

    // Wobble
    double phaseW = 0.0, phaseF = 0.0;
    double wowJitter = 0.0, wowAmpJitter = 1.0;
    double ouDrift = 0.0, bandState = 0.0;

    // Random (OU octave stack + post-LP)
    double ou1 = 0.0, ou2 = 0.0, ou3 = 0.0;
    double a1 = 0.0, a2 = 0.0, a3 = 0.0;
    double e1 = 0.0, e2 = 0.0, e3 = 0.0;
    double mLp = 0.0;

    // Glitch scheduler
    int    cellLen = 1, cellPos = 0;
    int    slotLen = 1;
    int    slotWait = 0;
    bool   slotArmed = false;
    bool   eventActive = false;
    int    eventType = 0, eventPos = 0, eventLen = 0;
    int    sliceLen = 0, slicePos = 0, sliceXf = 0;
    double stutterRate = 1.0, stutterRateMult = 1.0;
    bool   stutterRoll = false;
    int    chatterHalf = 1;
    double chatterAmp = 0.0;
    double shuffleAmount = 0.0;
    int    shuffleXf = 0;
    bool   shuffleEmitted = false;
    int    samplesSinceJump = 1 << 30;
    int    lastXfLen = 0;          // previous jump's fade length (jump guard)
    bool   snapEmitted = false;

    double rCur = 1.0;

    std::uint64_t noiseRng = kSeedNoise;
    std::uint64_t timeRng  = kSeedTime;
    std::uint64_t pickRng  = kSeedPick;
};
