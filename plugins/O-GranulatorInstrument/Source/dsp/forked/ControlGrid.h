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
// O-GranulatorInstrument (FORK-MANIFEST: ADAPTED): the parameter structs below
// bind to this plugin's APVTS through GiParamIds.
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../GiParamIds.h"

// ============================================================================
// Control grid (ARCHITECTURE §16, PLAN-2.1 T2).
//
// Parameters are read at ABSOLUTE 32-sample grid points only. Audible values
// interpolate linearly inside a cell from the previous grid value to the
// current one (one-cell lag). Every value at sample n therefore depends only
// on data before the grid point at or below n — block-size invariant.
// ============================================================================
namespace ogi
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

        /** 2.5a: a settled one-cell segment prev → cur (the matrix substitutes of
            ramped destinations; value (i) is then the CellLerp of the pair). */
        void setCell (float p, float c) noexcept
        {
            prev = p;
            cur = target = c;
            step = 0.0f;
            remaining = 0;
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
    // O-GranulatorInstrument (ADAPTED, Phases 2.1 / 2.2): the parent's
    // ParamPointers / GridParams are replaced by the instrument's own structs.
    // They cover the global rows and the stream subset the engine reads
    // (PLAN-2.1 T2, PLAN-2.2 T4). 2.4 / 2.5 add feedback, reverb, sync and the
    // matrix.
    //
    // Read since 2.1: polyphony, pitch_bend_range, grain_limit, output_gain,
    // amp_attack / decay / sustain / release, vel_to_amp, and per stream
    // sN_enable, mute, level, pan, position, scan_rate, scan_mode, loop_start,
    // loop_end, root_note, root_fine, key_track, size, size_rand, density,
    // pitch.
    // 2.2: s1...s4 (kGiGridStreams 4) + sN_solo, spray, jitter, pitch_rand,
    // scale, reverse_prob, window, window_skew, stereo_spread, gain_rand.
    // 2.3: the 10 per-grain FX rows sN_filter_type, filter_cutoff,
    // filter_rand, filter_res, drive, crush, crush_prob, ring_freq, ring_rand,
    // ring_mix (PLAN-2.3 T4; the parent planFx field names and conversions).
    // 2.4: the 9 global rows feedback, fb_low_cut, fb_high_cut, fb_saturation,
    // fb_diffusion, freeze, reverb_mix, reverb_size, reverb_damp (PLAN-2.4 T5;
    // the cutoffs raw Hz from the skewed ranges, never inverted by hand).
    // 2.5a (PLAN-2.5a T3, PD2 / PD15): the 89 mod / sync rows — sync_mode,
    // sN_size_sync / sN_density_sync, menvN_attack / decay / sustain / release,
    // lfoN_shape / rate / rate_sync / phase / mode, macroN, modM_source / dest /
    // depth. `struct Lfo` carries the parent field names and types
    // (82406fce ControlGrid.h:326-331) so the VERBATIM ModSources.h compiles
    // through `using GridParams = GiGridParams`.
    // R1 (PLAN-R1 PD13 / PD14, OS1): the 8 global feedback / reverb rows of 2.4
    // are per stream (sN_feedback, sN_fb_low_cut, sN_fb_high_cut,
    // sN_fb_saturation, sN_fb_diffusion, sN_reverb_mix, sN_reverb_size,
    // sN_reverb_damp) with sN_fb_enable (bool) and sN_reverb_pre (choice
    // 0 Post, 1 Pre); freeze stays global. kGiModDests 150.
    // 2.5b (PLAN-2.5b T3, PD15): voice_mode (choice index), glide (ms, read
    // raw through the skewed range), mpe_enable (bool), latched at the tick.
    // Never bound after 2.5b: rec_* (2.6).
    //==========================================================================
    inline constexpr int kGiGridStreams = 4;   // 2.2: s1...s4 (2.1: s1 only)
    inline constexpr int kGiLfos = 4, kGiModEnvs = 2, kGiMacros = 4, kGiModSlots = 16;
    inline constexpr int kGiModSources = 18;   // GiParamIds::kNumModSources (incl. None)
    inline constexpr int kGiModDests   = 150;  // GiParamIds::kModDests (incl. None)

    /** Raw APVTS atomics, cached once in the processor constructor (bind). A
        null pointer is simply not read (the GiGridParams default stays). */
    struct GiParamPointers
    {
        std::atomic<float>* polyphony      = nullptr;
        std::atomic<float>* pitchBendRange = nullptr;
        std::atomic<float>* voiceMode      = nullptr;   // 2.5b: choice 0 Poly, 1 Mono, 2 Legato
        std::atomic<float>* glide          = nullptr;   // 2.5b: ms (skewed range: read raw)
        std::atomic<float>* mpeEnable      = nullptr;   // 2.5b: bool
        std::atomic<float>* grainLimit     = nullptr;   // choice 0..3 → 32 / 64 / 128 / 256
        std::atomic<float>* outputGain     = nullptr;

        std::atomic<float>* ampAttack  = nullptr;      // ms (skewed range: read raw)
        std::atomic<float>* ampDecay   = nullptr;
        std::atomic<float>* ampSustain = nullptr;      // %
        std::atomic<float>* ampRelease = nullptr;
        std::atomic<float>* velToAmp   = nullptr;      // %

        // 2.4 global (R1: feedback and reverb are per stream)
        std::atomic<float>* freeze       = nullptr;    // bool

        struct Stream
        {
            std::atomic<float>* enable    = nullptr;
            std::atomic<float>* mute      = nullptr;
            std::atomic<float>* level     = nullptr;
            std::atomic<float>* pan       = nullptr;
            std::atomic<float>* position  = nullptr;
            std::atomic<float>* scanRate  = nullptr;
            std::atomic<float>* scanMode  = nullptr;   // choice 0 Free-run, 1 Retrigger, 2 Ping-pong
            std::atomic<float>* loopStart = nullptr;
            std::atomic<float>* loopEnd   = nullptr;
            std::atomic<float>* rootNote  = nullptr;
            std::atomic<float>* rootFine  = nullptr;
            std::atomic<float>* keyTrack  = nullptr;
            std::atomic<float>* size      = nullptr;
            std::atomic<float>* sizeRand  = nullptr;
            std::atomic<float>* density   = nullptr;
            std::atomic<float>* pitch     = nullptr;
            // 2.2
            std::atomic<float>* solo         = nullptr;
            std::atomic<float>* spray        = nullptr;
            std::atomic<float>* jitter       = nullptr;
            std::atomic<float>* pitchRand    = nullptr;
            std::atomic<float>* scale        = nullptr;   // choice 0 Off ... 31 Custom
            std::atomic<float>* reverseProb  = nullptr;
            std::atomic<float>* window       = nullptr;
            std::atomic<float>* windowSkew   = nullptr;
            std::atomic<float>* stereoSpread = nullptr;
            std::atomic<float>* gainRand     = nullptr;
            // 2.3
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
            // 2.5a
            std::atomic<float>* sizeSync     = nullptr;   // choice 0…19
            std::atomic<float>* densitySync  = nullptr;   // choice 0…18
            // R1: the stream's feedback and reverb
            std::atomic<float>* fbEnable     = nullptr;   // bool
            std::atomic<float>* feedback     = nullptr;   // %
            std::atomic<float>* fbLowCut     = nullptr;   // Hz (skewed range: read raw)
            std::atomic<float>* fbHighCut    = nullptr;   // Hz (skewed range: read raw)
            std::atomic<float>* fbSaturation = nullptr;   // %
            std::atomic<float>* fbDiffusion  = nullptr;   // %
            std::atomic<float>* reverbPre    = nullptr;   // choice 0 Post, 1 Pre
            std::atomic<float>* reverbMix    = nullptr;   // %
            std::atomic<float>* reverbSize   = nullptr;   // %
            std::atomic<float>* reverbDamp   = nullptr;   // %
        };
        Stream s[kGiGridStreams] {};

        // 2.5a: sync, sources, matrix
        std::atomic<float>* syncMode = nullptr;           // choice 0 Free, 1 Tempo
        struct ModEnv
        {
            std::atomic<float>* attack = nullptr;         // ms (skewed range: read raw)
            std::atomic<float>* decay = nullptr;
            std::atomic<float>* sustain = nullptr;        // %
            std::atomic<float>* release = nullptr;
        };
        ModEnv menv[kGiModEnvs] {};
        struct Lfo
        {
            std::atomic<float>* shape = nullptr;          // choice 0 Sine … 5 Smooth Random
            std::atomic<float>* rate = nullptr;           // Hz (skewed range: read raw)
            std::atomic<float>* rateSync = nullptr;       // choice 0…21
            std::atomic<float>* phase = nullptr;          // degrees
            std::atomic<float>* mode = nullptr;           // choice 0 Global, 1 Per-voice
        };
        Lfo lfo[kGiLfos] {};
        std::atomic<float>* macro[kGiMacros] {};          // %
        struct Mod
        {
            std::atomic<float>* source = nullptr;         // choice 0 None … 17 Macro 4
            std::atomic<float>* dest = nullptr;           // choice 0 None … 149
            std::atomic<float>* depth = nullptr;          // −100..100 %
        };
        Mod mod[kGiModSlots] {};

        /** Message thread (processor constructor). */
        void bind (juce::AudioProcessorValueTreeState& apvts)
        {
            polyphony      = apvts.getRawParameterValue ("polyphony");
            pitchBendRange = apvts.getRawParameterValue ("pitch_bend_range");
            voiceMode      = apvts.getRawParameterValue ("voice_mode");
            glide          = apvts.getRawParameterValue ("glide");
            mpeEnable      = apvts.getRawParameterValue ("mpe_enable");
            grainLimit     = apvts.getRawParameterValue ("grain_limit");
            outputGain     = apvts.getRawParameterValue ("output_gain");
            ampAttack      = apvts.getRawParameterValue ("amp_attack");
            ampDecay       = apvts.getRawParameterValue ("amp_decay");
            ampSustain     = apvts.getRawParameterValue ("amp_sustain");
            ampRelease     = apvts.getRawParameterValue ("amp_release");
            velToAmp       = apvts.getRawParameterValue ("vel_to_amp");
            freeze         = apvts.getRawParameterValue ("freeze");

            for (int i = 0; i < kGiGridStreams; ++i)
            {
                const int n = i + 1;
                auto& ps = s[i];
                ps.enable    = apvts.getRawParameterValue (GiParamIds::stream (n, "enable"));
                ps.mute      = apvts.getRawParameterValue (GiParamIds::stream (n, "mute"));
                ps.level     = apvts.getRawParameterValue (GiParamIds::stream (n, "level"));
                ps.pan       = apvts.getRawParameterValue (GiParamIds::stream (n, "pan"));
                ps.position  = apvts.getRawParameterValue (GiParamIds::stream (n, "position"));
                ps.scanRate  = apvts.getRawParameterValue (GiParamIds::stream (n, "scan_rate"));
                ps.scanMode  = apvts.getRawParameterValue (GiParamIds::stream (n, "scan_mode"));
                ps.loopStart = apvts.getRawParameterValue (GiParamIds::stream (n, "loop_start"));
                ps.loopEnd   = apvts.getRawParameterValue (GiParamIds::stream (n, "loop_end"));
                ps.rootNote  = apvts.getRawParameterValue (GiParamIds::stream (n, "root_note"));
                ps.rootFine  = apvts.getRawParameterValue (GiParamIds::stream (n, "root_fine"));
                ps.keyTrack  = apvts.getRawParameterValue (GiParamIds::stream (n, "key_track"));
                ps.size      = apvts.getRawParameterValue (GiParamIds::stream (n, "size"));
                ps.sizeRand  = apvts.getRawParameterValue (GiParamIds::stream (n, "size_rand"));
                ps.density   = apvts.getRawParameterValue (GiParamIds::stream (n, "density"));
                ps.pitch     = apvts.getRawParameterValue (GiParamIds::stream (n, "pitch"));

                ps.solo         = apvts.getRawParameterValue (GiParamIds::stream (n, "solo"));
                ps.spray        = apvts.getRawParameterValue (GiParamIds::stream (n, "spray"));
                ps.jitter       = apvts.getRawParameterValue (GiParamIds::stream (n, "jitter"));
                ps.pitchRand    = apvts.getRawParameterValue (GiParamIds::stream (n, "pitch_rand"));
                ps.scale        = apvts.getRawParameterValue (GiParamIds::stream (n, "scale"));
                ps.reverseProb  = apvts.getRawParameterValue (GiParamIds::stream (n, "reverse_prob"));
                ps.window       = apvts.getRawParameterValue (GiParamIds::stream (n, "window"));
                ps.windowSkew   = apvts.getRawParameterValue (GiParamIds::stream (n, "window_skew"));
                ps.stereoSpread = apvts.getRawParameterValue (GiParamIds::stream (n, "stereo_spread"));
                ps.gainRand     = apvts.getRawParameterValue (GiParamIds::stream (n, "gain_rand"));

                ps.filterType   = apvts.getRawParameterValue (GiParamIds::stream (n, "filter_type"));
                ps.filterCutoff = apvts.getRawParameterValue (GiParamIds::stream (n, "filter_cutoff"));
                ps.filterRand   = apvts.getRawParameterValue (GiParamIds::stream (n, "filter_rand"));
                ps.filterRes    = apvts.getRawParameterValue (GiParamIds::stream (n, "filter_res"));
                ps.drive        = apvts.getRawParameterValue (GiParamIds::stream (n, "drive"));
                ps.crush        = apvts.getRawParameterValue (GiParamIds::stream (n, "crush"));
                ps.crushProb    = apvts.getRawParameterValue (GiParamIds::stream (n, "crush_prob"));
                ps.ringFreq     = apvts.getRawParameterValue (GiParamIds::stream (n, "ring_freq"));
                ps.ringRand     = apvts.getRawParameterValue (GiParamIds::stream (n, "ring_rand"));
                ps.ringMix      = apvts.getRawParameterValue (GiParamIds::stream (n, "ring_mix"));

                ps.sizeSync     = apvts.getRawParameterValue (GiParamIds::stream (n, "size_sync"));
                ps.densitySync  = apvts.getRawParameterValue (GiParamIds::stream (n, "density_sync"));

                ps.fbEnable     = apvts.getRawParameterValue (GiParamIds::stream (n, "fb_enable"));
                ps.feedback     = apvts.getRawParameterValue (GiParamIds::stream (n, "feedback"));
                ps.fbLowCut     = apvts.getRawParameterValue (GiParamIds::stream (n, "fb_low_cut"));
                ps.fbHighCut    = apvts.getRawParameterValue (GiParamIds::stream (n, "fb_high_cut"));
                ps.fbSaturation = apvts.getRawParameterValue (GiParamIds::stream (n, "fb_saturation"));
                ps.fbDiffusion  = apvts.getRawParameterValue (GiParamIds::stream (n, "fb_diffusion"));
                ps.reverbPre    = apvts.getRawParameterValue (GiParamIds::stream (n, "reverb_pre"));
                ps.reverbMix    = apvts.getRawParameterValue (GiParamIds::stream (n, "reverb_mix"));
                ps.reverbSize   = apvts.getRawParameterValue (GiParamIds::stream (n, "reverb_size"));
                ps.reverbDamp   = apvts.getRawParameterValue (GiParamIds::stream (n, "reverb_damp"));
            }

            syncMode = apvts.getRawParameterValue ("sync_mode");
            for (int i = 0; i < kGiModEnvs; ++i)
            {
                const juce::String b = "menv" + juce::String (i + 1) + "_";
                menv[i].attack  = apvts.getRawParameterValue (b + "attack");
                menv[i].decay   = apvts.getRawParameterValue (b + "decay");
                menv[i].sustain = apvts.getRawParameterValue (b + "sustain");
                menv[i].release = apvts.getRawParameterValue (b + "release");
            }
            for (int i = 0; i < kGiLfos; ++i)
            {
                const juce::String b = "lfo" + juce::String (i + 1) + "_";
                lfo[i].shape    = apvts.getRawParameterValue (b + "shape");
                lfo[i].rate     = apvts.getRawParameterValue (b + "rate");
                lfo[i].rateSync = apvts.getRawParameterValue (b + "rate_sync");
                lfo[i].phase    = apvts.getRawParameterValue (b + "phase");
                lfo[i].mode     = apvts.getRawParameterValue (b + "mode");
            }
            for (int i = 0; i < kGiMacros; ++i)
                macro[i] = apvts.getRawParameterValue ("macro" + juce::String (i + 1));
            for (int m = 0; m < kGiModSlots; ++m)
            {
                const juce::String b = "mod" + juce::String (m + 1) + "_";
                mod[m].source = apvts.getRawParameterValue (b + "source");
                mod[m].dest   = apvts.getRawParameterValue (b + "dest");
                mod[m].depth  = apvts.getRawParameterValue (b + "depth");
            }
        }
    };

    //==========================================================================
    /** Every parameter value the engine uses, latched at a grid point.
        Engineering units, already converted: percentages → [0, 1], pan → [−1, 1],
        scan_rate → [−4, 4] (1 = real time), window_skew → [−1, 1], grain_limit
        choice → {32, 64, 128, 256}, choices → index. Non-finite reads hold the
        last value; reads are clamped to the parameter ranges (params.tsv).
        Defaults = the spec defaults. */
    struct GiGridParams
    {
        int   polyphony        = 8;        // 1..16
        float pitchBendRangeSt = 2.0f;     // 0..48
        int   voiceMode        = 0;        // 2.5b: 0 Poly, 1 Mono, 2 Legato
        float glideMs          = 0.0f;     // 2.5b: 0..2000
        bool  mpeEnable        = false;    // 2.5b
        int   grainLimit       = 256;      // 32 / 64 / 128 / 256
        float outputGainDb     = 0.0f;     // −24..12

        float ampAttackMs  = 20.0f;        // 0..10000
        float ampDecayMs   = 300.0f;       // 0..10000
        float ampSustain   = 1.0f;         // 0..1
        float ampReleaseMs = 1500.0f;      // 0..20000
        float velToAmp     = 0.7f;         // 0..1

        // 2.4 global (R1: feedback and reverb are per stream)
        bool  freeze       = false;

        struct Stream
        {
            bool  enable     = false;      // s1 defaults On through the APVTS
            bool  mute       = false;
            float levelDb    = 0.0f;       // −60..6
            float pan        = 0.0f;       // −1..1
            float position   = 0.0f;       // 0..1
            float scanRate   = 0.0f;       // −4..4 (1 = 100 % = real time)
            int   scanMode   = 1;          // 0 Free-run, 1 Retrigger, 2 Ping-pong
            float loopStart  = 0.0f;       // 0..1
            float loopEnd    = 1.0f;       // 0..1
            float rootNote   = 48.0f;      // 0..127
            float rootFineCt = 0.0f;       // −100..100 cents
            float keyTrack   = 1.0f;       // 0..1
            float sizeMs     = 100.0f;     // 5..2000
            float sizeRand   = 0.0f;       // 0..1
            float density    = 10.0f;      // 0.5..200 grains/s
            float pitchSt    = 0.0f;       // −24..24
            // 2.2
            bool  solo        = false;
            float spray       = 0.1f;      // 0..1
            float jitter      = 0.2f;      // 0..1
            float pitchRand   = 0.0f;      // 0..24 st
            int   scale       = 0;         // 0 Off, 1...31 (spec index; 31 = Custom)
            float reverseProb = 0.0f;      // 0..1
            float window      = 0.0f;      // 0..1 → WindowMorphBank step round (·64)
            float windowSkew  = 0.0f;      // −1..1 → tilt peak 0.5 + 0.45·skew
            float spread      = 0.3f;      // 0..1 (stereo_spread)
            float gainRand    = 0.0f;      // 0..1 (× 24 dB, cut only)
            // 2.3 per-grain FX (the parent planFx names)
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
            // 2.5a
            int   sizeSync       = 7;        // 0…19 (7 = 1/16)
            int   densitySync    = 7;        // 0…18 (7 = 1/16)
            // R1: the stream's feedback and reverb (spec defaults)
            bool  fbOn           = true;     // sN_fb_enable
            float feedback       = 0.0f;     // 0..1
            float fbLowCutHz     = 80.0f;    // 20..2000 Hz (raw)
            float fbHighCutHz    = 12000.0f; // 1000..20000 Hz (raw)
            float fbSaturation   = 0.2f;     // 0..1
            float fbDiffusion    = 0.0f;     // 0..1
            bool  reverbPre      = false;    // sN_reverb_pre: false Post, true Pre
            float reverbMix      = 0.0f;     // 0..1
            float reverbSize     = 0.5f;     // 0..1
            float reverbDamp     = 0.5f;     // 0..1
        };
        Stream s[kGiGridStreams] {};

        // 2.5a — spec defaults
        int syncMode = 0;                    // 0 Free, 1 Tempo
        struct Lfo                           // parent ControlGrid.h:326-331 (names + types)
        {
            int   shape    = 0;              // 0 Sine … 5 Smooth Random
            float rateHz   = 0.5f;           // 0.01..20 (raw)
            int   rateSync = 18;             // 0…21 (18 = 1 bar)
            float phaseDeg = 0.0f;           // 0..360
        };
        Lfo   lfo[kGiLfos] {};
        int   lfoMode[kGiLfos] {};           // 0 Global, 1 Per-voice
        struct ModEnv
        {
            float attackMs  = 10.0f;         // 0..10000 (raw)
            float decayMs   = 500.0f;        // 0..10000
            float sustain   = 0.0f;          // 0..1
            float releaseMs = 500.0f;        // 0..20000
        };
        ModEnv menv[kGiModEnvs] {};
        float macro[kGiMacros] {};           // 0..1
        struct Mod
        {
            int   src   = 0;                 // 0 None … 17 Macro 4
            int   dest  = 0;                 // 0 None … 149
            float depth = 0.0f;              // −1..1
        };
        Mod mod[kGiModSlots] {};

        void latch (const GiParamPointers& p) noexcept
        {
            readInt (p.polyphony, polyphony, 1, 16);
            read (p.pitchBendRange, pitchBendRangeSt, 0.0f, 48.0f);
            readInt (p.voiceMode, voiceMode, 0, 2);
            read (p.glide, glideMs, 0.0f, 2000.0f);
            readBool (p.mpeEnable, mpeEnable);
            if (p.grainLimit != nullptr)
            {
                const float v = p.grainLimit->load (std::memory_order_relaxed);
                if (std::isfinite (v))
                    grainLimit = 32 << std::clamp (static_cast<int> (std::lround (v)), 0, 3);
            }
            read (p.outputGain, outputGainDb, -24.0f, 12.0f);

            read (p.ampAttack,  ampAttackMs,  0.0f, 10000.0f);
            read (p.ampDecay,   ampDecayMs,   0.0f, 10000.0f);
            readPct (p.ampSustain, ampSustain);
            read (p.ampRelease, ampReleaseMs, 0.0f, 20000.0f);
            readPct (p.velToAmp, velToAmp);

            readBool (p.freeze, freeze);

            for (int i = 0; i < kGiGridStreams; ++i)
            {
                const auto& ps = p.s[i];
                auto& st = s[i];
                readBool (ps.enable, st.enable);
                readBool (ps.mute,   st.mute);
                read (ps.level,     st.levelDb,    -60.0f,    6.0f);
                read (ps.pan,       st.pan,       -100.0f,  100.0f, 100.0f);
                readPct (ps.position, st.position);
                read (ps.scanRate,  st.scanRate,  -400.0f,  400.0f, 100.0f);
                readInt (ps.scanMode, st.scanMode, 0, 2);
                readPct (ps.loopStart, st.loopStart);
                readPct (ps.loopEnd,   st.loopEnd);
                read (ps.rootNote,  st.rootNote,     0.0f,  127.0f);
                read (ps.rootFine,  st.rootFineCt, -100.0f, 100.0f);
                readPct (ps.keyTrack, st.keyTrack);
                read (ps.size,      st.sizeMs,       5.0f, 2000.0f);
                readPct (ps.sizeRand, st.sizeRand);
                read (ps.density,   st.density,      0.5f,  200.0f);
                read (ps.pitch,     st.pitchSt,    -24.0f,   24.0f);

                readBool (ps.solo, st.solo);
                readPct (ps.spray,  st.spray);
                readPct (ps.jitter, st.jitter);
                read (ps.pitchRand, st.pitchRand,    0.0f,   24.0f);
                readInt (ps.scale,  st.scale, 0, 31);
                readPct (ps.reverseProb, st.reverseProb);
                read (ps.window,     st.window,      0.0f,    1.0f);
                read (ps.windowSkew, st.windowSkew, -100.0f, 100.0f, 100.0f);
                readPct (ps.stereoSpread, st.spread);
                readPct (ps.gainRand,     st.gainRand);

                readInt (ps.filterType, st.filterType, 0, 3);
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
                readInt (ps.sizeSync,    st.sizeSync,    0, 19);
                readInt (ps.densitySync, st.densitySync, 0, 18);
                // R1
                readBool (ps.fbEnable, st.fbOn);
                readPct (ps.feedback, st.feedback);
                read (ps.fbLowCut,  st.fbLowCutHz,  20.0f,  2000.0f);
                read (ps.fbHighCut, st.fbHighCutHz, 1000.0f, 20000.0f);
                readPct (ps.fbSaturation, st.fbSaturation);
                readPct (ps.fbDiffusion,  st.fbDiffusion);
                readBool (ps.reverbPre, st.reverbPre);
                readPct (ps.reverbMix,  st.reverbMix);
                readPct (ps.reverbSize, st.reverbSize);
                readPct (ps.reverbDamp, st.reverbDamp);
            }

            // 2.5a: sync, sources, matrix (choice indices with lround, clamped).
            readInt (p.syncMode, syncMode, 0, 1);
            for (int n = 0; n < kGiLfos; ++n)
            {
                readInt (p.lfo[n].shape,    lfo[n].shape,    0, 5);
                read    (p.lfo[n].rate,     lfo[n].rateHz,   0.01f, 20.0f);
                readInt (p.lfo[n].rateSync, lfo[n].rateSync, 0, kTempoDivs - 1);
                read    (p.lfo[n].phase,    lfo[n].phaseDeg, 0.0f, 360.0f);
                readInt (p.lfo[n].mode,     lfoMode[n],      0, 1);
            }
            for (int n = 0; n < kGiModEnvs; ++n)
            {
                read    (p.menv[n].attack,  menv[n].attackMs,  0.0f, 10000.0f);
                read    (p.menv[n].decay,   menv[n].decayMs,   0.0f, 10000.0f);
                readPct (p.menv[n].sustain, menv[n].sustain);
                read    (p.menv[n].release, menv[n].releaseMs, 0.0f, 20000.0f);
            }
            for (int n = 0; n < kGiMacros; ++n)
                readPct (p.macro[n], macro[n]);
            for (int m = 0; m < kGiModSlots; ++m)
            {
                readInt (p.mod[m].source, mod[m].src,  0, kGiModSources - 1);
                readInt (p.mod[m].dest,   mod[m].dest, 0, kGiModDests - 1);
                read    (p.mod[m].depth,  mod[m].depth, -100.0f, 100.0f, 100.0f);
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

        static void readInt (const std::atomic<float>* src, int& dst, int lo, int hi) noexcept
        {
            if (src == nullptr)
                return;
            const float v = src->load (std::memory_order_relaxed);
            if (std::isfinite (v))
                dst = std::clamp (static_cast<int> (std::lround (v)), lo, hi);
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

    /** 2.5a (PLAN-2.5a PD2): the parent leaves (ModSources.h, TempoTracker.h)
        compile VERBATIM against the instrument's grid through this alias. */
    using GridParams = GiGridParams;
} // namespace ogi
