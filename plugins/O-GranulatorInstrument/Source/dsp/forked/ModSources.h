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
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include "CounterRng.h"        // grain-engine 1.1.0
#include "ControlGrid.h"
#include "EngineConstants.h"
#include "TempoTracker.h"

// ============================================================================
// Modulation sources (Stage 2 / Phase 2.5a, PLAN-2.5a P51; RESEARCH-2.5a R50,
// R51, R52; CONTEXT-2.5a D15 as amended by R50).
//
// Every source is SAMPLED ONCE PER CELL, at the grid tick (block-size invariant
// by construction). Ramped destinations lerp inside the cell; spawn-latched
// destinations lerp the matrix offset at the spawn sample.
//
// LFO phase (D15 / R50, signed off):
//   Free  → always the ACCUMULATOR {int64 cycle; double frac}, advanced once per
//           cell by 32·rate_Hz/fs (rate_Hz = the matrix-modulated rate of the
//           PREVIOUS cell — no same-cell source cycle, R50.5). Rate automation
//           can never jump the phase.
//   Tempo → DIRECT φ = frac ((ppq − barOrigin)/divBeats) on the held anchor
//           while the rate is unmodulated (the accumulator is re-seeded from it
//           every Direct cell). A rate offset ≠ 0 flips Direct → Accum
//           (increment 32·(pps/divBeats)·2^(4·off), continuous); Accum → Direct
//           only at a RE-LOCK (transport start, loop, locate, prepare, Free →
//           Tempo).
//   lfoN_phase is OUTSIDE the accumulator: out = shape (frac (acc + φ_ramped/360)),
//   φ ramped over 20 ms on the shortest arc (359° → 1° moves +2°).
// Shapes (R51, bipolar): sine, triangle 1 − 4|x − ½|, saw 2x − 1, square,
//   S&H 2h(c) − 1, smooth random 2·lerp_cos (h(c), h(c+1), x) − 1, with
//   h(c) = Draw (seed, kLfoStream + N, uint64 (c)).nextFloat (0). c is the cycle
//   of the PHASED position (negative in pre-roll → a large uint64: deterministic).
//
// Envelope follower (R52): tap 0.5·(L + R)·inGain(i) at the capture input (post
// input_gain, pre-ring; never the feedback return or the dry path; runs while
// frozen). Per sample x = |s·envGain|, e += (x > e ? aA : aR)·(x − e),
// a = 1 − exp (−1/(τ·fs)) in double → float at ticks. Once per piece a
// non-finite e resets to 0. Source = clamp ((20·log10 (max (e, 1e-30)) + 60)/60,
// 0, 1) at the tick — silence is exactly 0.
//
// Macros: macroN/100 through an ExactRamp (20 ms). Unipolar.
// Pitch (6) and Onset (7): live from 2.5b, computed in Analysis.h (the Pitch
// source slews the tracked note, the Onset source decays from the last event)
// and sampled at the tick like every other source.
// ============================================================================
namespace ogi
{
    namespace ModSrc
    {
        enum : int { None = 0, Lfo1 = 1, Lfo4 = 4, Env = 5, Pitch = 6, Onset = 7, Macro1 = 8, Macro4 = 11 };
    }

    /** The source vector of one cell: v[0] = None = 0, v[1…11]. */
    struct SourceValues
    {
        float v[kModSources] {};
    };

    //==========================================================================
    class Lfo
    {
    public:
        enum class Mode { Direct, Accum };

        /** Message thread / prepare: snap the phase ramp; the next tick locks. */
        void prepare (double sampleRate, int rampCellsIn, float phaseDeg) noexcept
        {
            fs = sampleRate;
            rampCells = rampCellsIn;
            phaseRamp.reset (phaseDeg);
            phaseTarget = phaseDeg;
            cycle = 0;
            frac = 0.0;
            mode = Mode::Accum;
            lockSeen = ~std::uint64_t (0);
            wasTempo = false;
            out = 0.0f;
        }

        /** Grid tick. `rateOffset` = the matrix offset on this LFO's rate from the
            PREVIOUS cell (normalised); `rateHz` = the modulated (or raw) rate in Hz
            from the previous cell. Returns the value for THIS cell. forceMode:
            test hook (0 off, 1 Direct, 2 Accum). */
        float tick (int index, const GridParams::Lfo& p, bool tempo, const TempoTracker& tr, std::int64_t tickAbs,
                    float rateOffset, double rateHz, std::uint64_t seed, int forceMode = 0) noexcept
        {
            // Phase parameter: shortest arc, 20 ms.
            {
                double d = static_cast<double> (p.phaseDeg) - wrap360 (static_cast<double> (phaseTarget));
                d -= 360.0 * std::round (d / 360.0);
                const float want = phaseTarget + static_cast<float> (d);
                if (! juce::exactlyEqual (want, phaseTarget) && std::abs (d) > 0.0)
                {
                    phaseTarget = want;
                    phaseRamp.setTarget (phaseTarget, rampCells);
                }
                phaseRamp.tick();
            }

            const TempoAnchor& A = tr.anchor();
            const double divBeats = tempoDivBeats (p.rateSync, A.barLen);
            const bool modulated = ! juce::exactlyEqual (rateOffset, 0.0f);

            // Mode selection.
            const std::uint64_t lock = tr.getRelockEpoch();
            const bool relock = tempo && (lock != lockSeen || ! wasTempo);
            lockSeen = lock;
            wasTempo = tempo;
            if (! tempo)
                mode = Mode::Accum;
            else if (relock)
                mode = modulated ? Mode::Accum : Mode::Direct;
            else if (modulated)
                mode = Mode::Accum;   // Direct → Accum on the predicate; Accum stays until a re-lock
            if (forceMode == 1) mode = Mode::Direct;
            if (forceMode == 2) mode = Mode::Accum;

            if (! tempo && forceMode == 1)
            {
                // LS2n: the Free "direct" form frac (n·inc) from the cell index (R50 control).
                const double v = static_cast<double> (tickAbs / kCell) * static_cast<double> (kCell) * rateHz / fs;
                const double c = std::floor (v);
                cycle = static_cast<std::int64_t> (c);
                frac  = v - c;
            }
            else if (mode == Mode::Direct || (tempo && relock))
            {
                const double v = (tr.ppqAt (tickAbs) - A.barOrigin) / divBeats;
                const double c = std::floor (v);
                cycle = static_cast<std::int64_t> (c);
                frac  = v - c;
                if (! (frac < 1.0)) { frac = 0.0; ++cycle; }
            }

            // Value at this cell.
            const double ph = frac + static_cast<double> (phaseRamp.current()) / 360.0;
            const double fl = std::floor (ph);
            const std::int64_t c = cycle + static_cast<std::int64_t> (fl);
            double x = ph - fl;
            if (! (x < 1.0)) x = 0.0;
            out = shape (p.shape, x, c, seed, index);
            posOut = static_cast<double> (c) + x;

            // Advance the accumulator for the next cell (Direct re-seeds next tick anyway).
            double inc = 0.0;
            if (tempo)
                inc = static_cast<double> (kCell) * (A.pps / divBeats) * std::exp2 (4.0 * static_cast<double> (rateOffset));
            else
                inc = static_cast<double> (kCell) * rateHz / fs;
            if (std::isfinite (inc) && inc > 0.0)
            {
                frac += inc;
                const double w = std::floor (frac);
                cycle += static_cast<std::int64_t> (w);
                frac -= w;
            }
            return out;
        }

        static float shape (int s, double x, std::int64_t c, std::uint64_t seed, int index) noexcept
        {
            auto h = [seed, index] (std::int64_t k) noexcept
            {
                return static_cast<double> (ouaricon::grain::CounterRng::Draw (seed, kLfoStream + static_cast<std::uint64_t> (index),
                                                                               static_cast<std::uint64_t> (k)).nextFloat (0));
            };
            switch (s)
            {
                case 1:  return static_cast<float> (1.0 - 4.0 * std::abs (x - 0.5));
                case 2:  return static_cast<float> (2.0 * x - 1.0);
                case 3:  return x < 0.5 ? 1.0f : -1.0f;
                case 4:  return static_cast<float> (2.0 * h (c) - 1.0);
                case 5:
                {
                    const double a = h (c), b = h (c + 1);
                    return static_cast<float> (2.0 * (a + (b - a) * 0.5 * (1.0 - std::cos (kFxPi * x))) - 1.0);
                }
                default: return static_cast<float> (std::sin (2.0 * kFxPi * x));
            }
        }

        Mode getMode() const noexcept          { return mode; }
        std::int64_t getCycle() const noexcept { return cycle; }
        double getFrac() const noexcept        { return frac; }
        float value() const noexcept           { return out; }
        /** The phased position of this cell's value: cycle + x (unwrapped). */
        double position() const noexcept       { return posOut; }
        float getPhaseDeg() const noexcept     { return phaseRamp.current(); }

    private:
        static double wrap360 (double d) noexcept { return d - 360.0 * std::floor (d / 360.0); }

        double fs = 48000.0;
        int rampCells = 30;
        ExactRamp phaseRamp;
        float phaseTarget = 0.0f;   // unwrapped
        std::int64_t cycle = 0;
        double frac = 0.0;
        Mode mode = Mode::Accum;
        std::uint64_t lockSeen = 0;
        bool wasTempo = false;
        float out = 0.0f;
        double posOut = 0.0;
    };

    //==========================================================================
    class EnvFollower
    {
    public:
        void prepare (double sampleRate, float attackMs, float releaseMs, float gainDb) noexcept
        {
            fs = sampleRate;
            e = 0.0f;
            setParams (attackMs, releaseMs, gainDb);
        }

        /** Grid tick: coefficients from the latched raw ms / dB. */
        void setParams (float attackMs, float releaseMs, float gainDb) noexcept
        {
            if (! juce::exactlyEqual (attackMs, lastA))
            {
                lastA = attackMs;
                aA = static_cast<float> (1.0 - std::exp (-1.0 / (static_cast<double> (attackMs) * 0.001 * fs)));
            }
            if (! juce::exactlyEqual (releaseMs, lastR))
            {
                lastR = releaseMs;
                aR = static_cast<float> (1.0 - std::exp (-1.0 / (static_cast<double> (releaseMs) * 0.001 * fs)));
            }
            g = dbToGain (gainDb);
        }

        /** One piece of the conditioned input: s[i] = 0.5·(L + R)·inGain(i). */
        template <typename GainAt>
        void process (const float* inL, const float* inR, int n, int cellPos, GainAt&& gainAt) noexcept
        {
            for (int i = 0; i < n; ++i)
            {
                const float s = 0.5f * (inL[i] + inR[i]) * gainAt (cellPos + i);
                float x = std::abs (s * g);
                // Stage 4 (D47 a, PLAN-4 P147): the R96 predicate — a non-finite or
                // absurd (> +80 dBFS) sample contributes 0, as at the ring write and
                // the analysis tap. Exactly a no-op for |s·g| <= kAnalysisTapMax.
               #if OGI_TEST_HOOKS
                if (! testPinOff)
               #endif
                if (! (x <= kAnalysisTapMax)) x = 0.0f;
                e += (x > e ? aA : aR) * (x - e);
            }
        }

        /** Once per piece. Returns true when a non-finite state was scrubbed. */
        bool scrub() noexcept
        {
            if (std::isfinite (e))
                return false;
            e = 0.0f;
            return true;
        }

        /** The source value at the tick. */
        float source() const noexcept
        {
            const double db = 20.0 * std::log10 (std::max (static_cast<double> (e), 1.0e-30));
            return static_cast<float> (std::clamp ((db + 60.0) / 60.0, 0.0, 1.0));
        }

        float state() const noexcept { return e; }
        float attackCoeff() const noexcept { return aA; }
        float releaseCoeff() const noexcept { return aR; }
       #if OGI_TEST_HOOKS
        void injectForTest (float v) noexcept { e = v; }
        bool testPinOff = false;   // Z7h3n (PLAN-4 P147): no kAnalysisTapMax predicate at the input
       #endif

    private:
        double fs = 48000.0;
        float e = 0.0f, aA = 1.0f, aR = 1.0f, g = 1.0f;
        float lastA = -1.0f, lastR = -1.0f;
    };
} // namespace ogi
