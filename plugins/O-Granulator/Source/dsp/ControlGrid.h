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

#include <juce_audio_basics/juce_audio_basics.h>   // juce::Decibels
#include <algorithm>
#include <atomic>
#include <cmath>
#include "EngineConstants.h"

// ============================================================================
// Control grid (ARCHITECTURE §16, PLAN-2.1 T2).
//
// Parameters are read at ABSOLUTE 32-sample grid points only. Audible values
// interpolate linearly inside a cell from the previous grid value to the
// current one (one-cell lag). Every value at sample n therefore depends only
// on data before the grid point at or below n — block-size invariant.
// ============================================================================
namespace ogran
{
    inline constexpr float kCellInv = 1.0f / static_cast<float> (kCell);   // exact (power of two)

    /** Ramp length in whole cells: ms rounded UP (parameter-derived, never block-derived).
        10 ms at 48 k = 15 cells (P12 mixer ramps); 20 ms = 30. */
    inline int rampCellsFor (double fs, double ms) noexcept
    {
        return std::max (1, static_cast<int> (std::ceil (ms / 1000.0 * fs / static_cast<double> (kCell))));
    }

    /** Base ramp length in whole cells: 20 ms (input / output gain, mix, g_out). */
    inline int rampCellsFor (double fs) noexcept
    {
        return rampCellsFor (fs, kBaseRampMs);
    }

    //==========================================================================
    /** A grid-rate value lerped inside the cell: prev = value at the previous
        grid point, cur = value at this one. at (i) for i in [0, kCell).
        prev == cur gives exactly cur at every i (x + 0·t == x). */
    struct CellLerp
    {
        float prev = 0.0f, cur = 0.0f;

        void reset (float v) noexcept       { prev = cur = v; }
        void push (float v) noexcept        { prev = cur; cur = v; }
        float at (int i) const noexcept     { return prev + (cur - prev) * (static_cast<float> (i) * kCellInv); }
        bool settledAt (float v) const noexcept { return juce::exactlyEqual (prev, v) && juce::exactlyEqual (cur, v); }
    };

    //==========================================================================
    /** Linear ramp advanced once per cell, with EXACT arrival: the tick that
        completes the ramp assigns current = target bit-for-bit.

        Not juce::SmoothedValue: its reset() copies the target into current, so a
        NaN target sticks (memory: SmoothedValue::reset does not clear a NaN).
        Here a non-finite target is IGNORED (the last value holds).

        Per grid tick:  setTarget (latched value, cells);  tick();
        Per sample i of the cell: value (i) lerps previous() → current(). */
    class ExactRamp
    {
    public:
        void reset (float v) noexcept
        {
            if (! std::isfinite (v))
                v = target;
            prev = cur = target = v;
            step = 0.0f;
            remaining = 0;
        }

        void setTarget (float t, int cells) noexcept
        {
            if (! std::isfinite (t) || juce::exactlyEqual (t, target))
                return;
            target    = t;
            remaining = std::max (1, cells);
            step      = (target - cur) / static_cast<float> (remaining);
        }

        /** Advance one cell (call once per grid point, after setTarget). */
        void tick() noexcept
        {
            prev = cur;
            if (remaining > 0)
            {
                --remaining;
                cur = (remaining == 0) ? target : cur + step;
            }
        }

        float value (int i) const noexcept   { return prev + (cur - prev) * (static_cast<float> (i) * kCellInv); }
        float current() const noexcept       { return cur; }
        float previous() const noexcept      { return prev; }
        float getTarget() const noexcept     { return target; }

        /** True when the whole current cell sits at exactly v. */
        bool settledAt (float v) const noexcept { return remaining == 0 && juce::exactlyEqual (prev, v) && juce::exactlyEqual (cur, v); }

    private:
        float prev = 0.0f, cur = 0.0f, target = 0.0f, step = 0.0f;
        int remaining = 0;
    };

    //==========================================================================
    // Gain laws (R2 / P3). Never derive unity from a normalised get(): read the
    // denormalised dB, snap |dB| < 1e-3 to exactly 0, then decibelsToGain.
    // A host normalised round-trip of the 0 dB default lands at 7.15e-7 dB,
    // which would otherwise give 1.00000012f and break the mix-0 bitwise null.
    inline float snapDb (float db) noexcept           { return std::abs (db) < 1.0e-3f ? 0.0f : db; }
    inline float dbToGain (float db) noexcept         { return juce::Decibels::decibelsToGain (snapDb (db)); }

    /** sN_level: −60 dB (UI prints −inf at <= −59.95) is silence, exactly 0. */
    inline float levelDbToGain (float db) noexcept    { return db <= -59.95f ? 0.0f : dbToGain (db); }

    struct GainPair { float a = 1.0f, b = 0.0f; };

    /** Equal-power dry/wet with SNAPPED ends: m == 0 → (1, 0), m == 1 → (0, 1)
        exactly; otherwise (cos ½πm, sin ½πm). a = dry, b = wet. */
    inline GainPair mixGains (float m) noexcept
    {
        if (! (m > 0.0f)) return { 1.0f, 0.0f };   // also NaN
        if (m >= 1.0f)    return { 0.0f, 1.0f };
        const double t = 0.5 * 3.14159265358979323846 * static_cast<double> (m);
        return { static_cast<float> (std::cos (t)), static_cast<float> (std::sin (t)) };
    }

    /** Constant-power balance for a stereo bus, unity at centre (snapped):
        θ = (p + 1)·π/4, (L, R) = √2·(cos θ, sin θ). p in [−1, 1]. a = L, b = R. */
    inline GainPair panGains (float p) noexcept
    {
        if (! (p < 0.0f || p > 0.0f)) return { 1.0f, 1.0f };   // centre (±0, and NaN) → exact unity
        const double pc = std::clamp (static_cast<double> (p), -1.0, 1.0);
        const double th = (pc + 1.0) * 0.25 * 3.14159265358979323846;
        constexpr double kSqrt2 = 1.41421356237309504880;
        return { static_cast<float> (kSqrt2 * std::cos (th)), static_cast<float> (kSqrt2 * std::sin (th)) };
    }

    //==========================================================================
    /** Raw APVTS atomics, cached once in the processor constructor. A null
        pointer is simply not read (the GridParams default stays). */
    struct ParamPointers
    {
        std::atomic<float>* inputGain    = nullptr;
        std::atomic<float>* bufferLength = nullptr;
        std::atomic<float>* mix          = nullptr;
        std::atomic<float>* outputGain   = nullptr;
        std::atomic<float>* grainLimit   = nullptr;
        // 2.4: freeze, feedback network, output reverb
        std::atomic<float>* freeze       = nullptr;
        std::atomic<float>* feedback     = nullptr;
        std::atomic<float>* fbLowCut     = nullptr;   // Hz (skewed range: read raw, never invert the skew)
        std::atomic<float>* fbHighCut    = nullptr;   // Hz (skewed range)
        std::atomic<float>* fbSaturation = nullptr;
        std::atomic<float>* fbDiffusion  = nullptr;
        std::atomic<float>* reverbMix    = nullptr;
        std::atomic<float>* reverbSize   = nullptr;
        std::atomic<float>* reverbDamp   = nullptr;

        struct Stream
        {
            std::atomic<float>* enable      = nullptr;
            std::atomic<float>* level       = nullptr;
            std::atomic<float>* pan         = nullptr;
            std::atomic<float>* position    = nullptr;
            std::atomic<float>* spray       = nullptr;
            std::atomic<float>* size        = nullptr;
            std::atomic<float>* sizeRand    = nullptr;
            std::atomic<float>* density     = nullptr;
            std::atomic<float>* jitter      = nullptr;
            std::atomic<float>* pitch       = nullptr;
            std::atomic<float>* pitchRand   = nullptr;
            std::atomic<float>* reverseProb = nullptr;
            // 2.2
            std::atomic<float>* mute        = nullptr;
            std::atomic<float>* solo        = nullptr;
            std::atomic<float>* scale       = nullptr;   // choice index 0 (Off) … 31 (Custom)
            std::atomic<float>* window      = nullptr;
            std::atomic<float>* windowSkew  = nullptr;
            std::atomic<float>* spread      = nullptr;
            std::atomic<float>* gainRand    = nullptr;
            // 2.3 per-grain FX
            std::atomic<float>* filterType   = nullptr;   // choice 0 Off, 1 LP, 2 BP, 3 HP
            std::atomic<float>* filterCutoff = nullptr;   // Hz (skewed range: read raw, never invert the skew)
            std::atomic<float>* filterRand   = nullptr;
            std::atomic<float>* filterRes    = nullptr;
            std::atomic<float>* drive        = nullptr;
            std::atomic<float>* crush        = nullptr;
            std::atomic<float>* crushProb    = nullptr;
            std::atomic<float>* ringFreq     = nullptr;   // Hz (skewed range)
            std::atomic<float>* ringRand     = nullptr;
            std::atomic<float>* ringMix      = nullptr;
            // 2.5a tempo sync (choice indices into the 20 / 19 division lists)
            std::atomic<float>* sizeSync     = nullptr;
            std::atomic<float>* densitySync  = nullptr;
        };
        Stream s[kMaxStreams] {};

        // 2.5a: sync, LFO / env / macro sources, modulation matrix.
        std::atomic<float>* syncMode   = nullptr;   // choice 0 Free, 1 Tempo
        struct Lfo
        {
            std::atomic<float>* shape    = nullptr;   // choice 0…5
            std::atomic<float>* rate     = nullptr;   // Hz (skewed range: read raw)
            std::atomic<float>* rateSync = nullptr;   // choice 0…21
            std::atomic<float>* phase    = nullptr;   // degrees
        };
        Lfo lfo[kLfos] {};
        std::atomic<float>* envAttack  = nullptr;   // ms (skewed)
        std::atomic<float>* envRelease = nullptr;   // ms (skewed)
        std::atomic<float>* envGain    = nullptr;   // dB
        std::atomic<float>* macro[kMacros] {};
        struct Mod
        {
            std::atomic<float>* source = nullptr;   // choice 0…11
            std::atomic<float>* dest   = nullptr;   // choice 0…112
            std::atomic<float>* depth  = nullptr;   // −100…100 %
        };
        Mod mod[kModSlots] {};
    };

    //==========================================================================
    /** Every parameter value the engine uses, latched at a grid point.
        Engineering units, already converted: percentages → [0, 1], pan and
        window_skew → [−1, 1], grain_limit choice → {32, 64, 96, 128}, scale →
        raw choice index 0…31. Non-finite reads hold the last
        value; reads are clamped to the parameter ranges. */
    struct GridParams
    {
        float inputGainDb   = 0.0f;
        float outputGainDb  = 0.0f;
        float mix           = 0.5f;    // 0..1
        float bufferLengthS = 8.0f;    // 1..30
        int   grainLimit    = 128;
        // 2.4 — spec defaults
        bool  freeze        = false;
        float feedback      = 0.0f;      // 0..1
        float fbLowCutHz    = 80.0f;     // 20..2000 Hz (raw)
        float fbHighCutHz   = 12000.0f;  // 1000..20000 Hz (raw)
        float fbSat         = 0.2f;      // 0..1
        float fbDiff        = 0.0f;      // 0..1
        float reverbMix     = 0.0f;      // 0..1
        float reverbSize    = 0.5f;      // 0..1
        float reverbDamp    = 0.5f;      // 0..1

        struct Stream
        {
            bool  enable      = false;
            float levelDb     = 0.0f;    // −60..6
            float pan         = 0.0f;    // −1..1
            float position    = 0.0f;    // 0..1
            float spray       = 0.1f;    // 0..1
            float sizeMs      = 100.0f;  // 5..2000
            float sizeRand    = 0.0f;    // 0..1
            float density     = 10.0f;   // 0.5..200 grains/s
            float jitter      = 0.2f;    // 0..1
            float pitch       = 0.0f;    // −24..24 st
            float pitchRand   = 0.0f;    // 0..24 st
            float reverseProb = 0.0f;    // 0..1
            // 2.2
            bool  mute        = false;
            bool  solo        = false;
            int   scale       = 0;       // 0 Off, 1…31 (spec index; 31 = Custom)
            float window      = 0.0f;    // 0..1 → WindowMorphBank step round (·64)
            float windowSkew  = 0.0f;    // −1..1 → tilt peak 0.5 + 0.45·skew
            float spread      = 0.3f;    // 0..1 (spec default 30 %)
            float gainRand    = 0.0f;    // 0..1 (D5: × 24 dB, cut only)
            // 2.3 per-grain FX — spec defaults; latched per grain at spawn
            int   filterType     = 0;        // 0 Off, 1 LP, 2 BP, 3 HP
            float filterCutoffHz = 20000.0f; // 20..20000 Hz (raw)
            float filterRand     = 0.0f;     // 0..1 (±2 oct at 1)
            float filterRes      = 0.2f;     // 0..1
            float drive          = 0.0f;     // 0..1
            float crush          = 0.0f;     // 0..1
            float crushProb      = 1.0f;     // 0..1
            float ringFreqHz     = 200.0f;   // 20..5000 Hz (raw)
            float ringRand       = 0.0f;     // 0..1
            float ringMix        = 0.0f;     // 0..1
            // 2.5a tempo sync — spec defaults (index 7 = 1/16)
            int   sizeSync       = 7;        // 0…19
            int   densitySync    = 7;        // 0…18
        };
        Stream s[kMaxStreams] {};

        // 2.5a — spec defaults
        int syncMode = 0;                    // 0 Free, 1 Tempo
        struct Lfo
        {
            int   shape    = 0;              // 0 Sine … 5 Smooth Random
            float rateHz   = 0.5f;           // 0.01..20 (raw)
            int   rateSync = 18;             // 0…21 (18 = 1 bar)
            float phaseDeg = 0.0f;           // 0..360
        };
        Lfo   lfo[kLfos] {};
        float envAttackMs  = 10.0f;          // 1..500
        float envReleaseMs = 200.0f;         // 10..2000
        float envGainDb    = 0.0f;           // −24..24
        float macro[kMacros] {};             // 0..1
        struct Mod
        {
            int   src   = 0;                 // 0 None … 11 Macro 4
            int   dest  = 0;                 // 0 None … 112
            float depth = 0.0f;              // −1..1
        };
        Mod mod[kModSlots] {};

        void latch (const ParamPointers& p) noexcept
        {
            read (p.inputGain,    inputGainDb,   -24.0f, 12.0f);
            read (p.outputGain,   outputGainDb,  -24.0f, 12.0f);
            readPct (p.mix,       mix);
            read (p.bufferLength, bufferLengthS,   1.0f, 30.0f);

            if (p.grainLimit != nullptr)
            {
                const float v = p.grainLimit->load (std::memory_order_relaxed);
                if (std::isfinite (v))
                    grainLimit = 32 * (1 + std::clamp (static_cast<int> (std::lround (v)), 0, 3));
            }

            readBool (p.freeze, freeze);
            readPct (p.feedback,     feedback);
            read (p.fbLowCut,     fbLowCutHz,    20.0f,  2000.0f);
            read (p.fbHighCut,    fbHighCutHz, 1000.0f, 20000.0f);
            readPct (p.fbSaturation, fbSat);
            readPct (p.fbDiffusion,  fbDiff);
            readPct (p.reverbMix,    reverbMix);
            readPct (p.reverbSize,   reverbSize);
            readPct (p.reverbDamp,   reverbDamp);

            for (int i = 0; i < kMaxStreams; ++i)
            {
                const auto& ps = p.s[i];
                auto& st = s[i];
                if (ps.enable != nullptr)
                {
                    const float v = ps.enable->load (std::memory_order_relaxed);
                    if (std::isfinite (v))
                        st.enable = v > 0.5f;
                }
                read (ps.level,      st.levelDb,   -60.0f,    6.0f);
                read (ps.pan,        st.pan,      -100.0f,  100.0f, 100.0f);
                readPct (ps.position,    st.position);
                readPct (ps.spray,       st.spray);
                read (ps.size,       st.sizeMs,      5.0f, 2000.0f);
                readPct (ps.sizeRand,    st.sizeRand);
                read (ps.density,    st.density,     0.5f,  200.0f);
                readPct (ps.jitter,      st.jitter);
                read (ps.pitch,      st.pitch,     -24.0f,   24.0f);
                read (ps.pitchRand,  st.pitchRand,   0.0f,   24.0f);
                readPct (ps.reverseProb, st.reverseProb);

                readBool (ps.mute, st.mute);
                readBool (ps.solo, st.solo);
                if (ps.scale != nullptr)
                {
                    const float v = ps.scale->load (std::memory_order_relaxed);
                    if (std::isfinite (v))
                        st.scale = std::clamp (static_cast<int> (std::lround (v)), 0, 31);
                }
                read (ps.window,     st.window,      0.0f,    1.0f);
                read (ps.windowSkew, st.windowSkew, -100.0f, 100.0f, 100.0f);
                readPct (ps.spread,      st.spread);
                readPct (ps.gainRand,    st.gainRand);

                if (ps.filterType != nullptr)
                {
                    const float v = ps.filterType->load (std::memory_order_relaxed);
                    if (std::isfinite (v))
                        st.filterType = std::clamp (static_cast<int> (std::lround (v)), 0, 3);
                }
                read (ps.filterCutoff, st.filterCutoffHz, 20.0f, 20000.0f);
                readPct (ps.filterRand,  st.filterRand);
                readPct (ps.filterRes,   st.filterRes);
                readPct (ps.drive,       st.drive);
                readPct (ps.crush,       st.crush);
                readPct (ps.crushProb,   st.crushProb);
                read (ps.ringFreq,     st.ringFreqHz,     20.0f,  5000.0f);
                readPct (ps.ringRand,    st.ringRand);
                readPct (ps.ringMix,     st.ringMix);
                // 2.5a
                readChoice (ps.sizeSync,    st.sizeSync,    19);
                readChoice (ps.densitySync, st.densitySync, 18);
            }

            // 2.5a: sync, sources, matrix (choice indices with lround, clamped).
            readChoice (p.syncMode, syncMode, 1);
            for (int n = 0; n < kLfos; ++n)
            {
                readChoice (p.lfo[n].shape,    lfo[n].shape,    5);
                read       (p.lfo[n].rate,     lfo[n].rateHz,   0.01f, 20.0f);
                readChoice (p.lfo[n].rateSync, lfo[n].rateSync, kTempoDivs - 1);
                read       (p.lfo[n].phase,    lfo[n].phaseDeg, 0.0f, 360.0f);
            }
            read (p.envAttack,  envAttackMs,   1.0f,  500.0f);
            read (p.envRelease, envReleaseMs, 10.0f, 2000.0f);
            read (p.envGain,    envGainDb,   -24.0f,   24.0f);
            for (int n = 0; n < kMacros; ++n)
                readPct (p.macro[n], macro[n]);
            for (int m = 0; m < kModSlots; ++m)
            {
                readChoice (p.mod[m].source, mod[m].src,  kModSources - 1);
                readChoice (p.mod[m].dest,   mod[m].dest, kModDests - 1);
                read       (p.mod[m].depth,  mod[m].depth, -100.0f, 100.0f, 100.0f);
            }
        }

    private:
        /** dst = clamp (v, lo, hi) / divisor. Division (not × 0.01f) so 0 and the
            range ends map exactly (100 / 100 == 1.0f, 0 / 100 == 0.0f). */
        static void read (const std::atomic<float>* src, float& dst, float lo, float hi, float divisor = 1.0f) noexcept
        {
            if (src == nullptr)
                return;
            const float v = src->load (std::memory_order_relaxed);
            if (std::isfinite (v))
                dst = std::clamp (v, lo, hi) / divisor;
        }

        static void readPct (const std::atomic<float>* src, float& dst) noexcept
        {
            read (src, dst, 0.0f, 100.0f, 100.0f);
        }

        static void readChoice (const std::atomic<float>* src, int& dst, int maxIndex) noexcept
        {
            if (src == nullptr)
                return;
            const float v = src->load (std::memory_order_relaxed);
            if (std::isfinite (v))
                dst = std::clamp (static_cast<int> (std::lround (v)), 0, maxIndex);
        }

        static void readBool (const std::atomic<float>* src, bool& dst) noexcept
        {
            if (src == nullptr)
                return;
            const float v = src->load (std::memory_order_relaxed);
            if (std::isfinite (v))
                dst = v > 0.5f;
        }
    };
} // namespace ogran
