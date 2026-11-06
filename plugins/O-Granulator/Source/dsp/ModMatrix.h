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
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <limits>
#include "ControlGrid.h"
#include "EngineConstants.h"
#include "ModSources.h"

// ============================================================================
// Modulation matrix (Stage 2 / Phase 2.5a, PLAN-2.5a P52–P55; RESEARCH-2.5a
// R46–R49, R55, R58).
//
// 16 slots {source, dest, depth} in NORMALISED space. Per grid tick:
//   1. compare the latched modM_source / dest / depth with the slot state (no
//      listeners). Depth only → the slot's 20 ms ExactRamp. Source or dest →
//      if the old route was live, its frozen contribution (depth·src of the
//      previous cell) is added to residual[oldDest], which ramps LINEARLY to
//      exactly 0 over 20 ms (R49, signed off: per-destination residual — any
//      change rate, value-continuous, fixed memory); the slot restarts at depth
//      0 and ramps to its target.
//   2. off[d] = Σ depth·src + residual[d], summed BEFORE the single clamp.
//   3. A destination is ROUTED iff off[d] ≠ 0 (−0 counts as 0). Unrouted
//      destinations take the raw 2.4 path untouched (R46: the normalised round
//      trip is NOT identity on skewed ranges — s_level 25 %, size 59–66 %).
//      Routed: v01 = clamp (base01 + off, 0, 1), raw = range.convertFrom0to1 (v01).
// Ranges are the contract rows (start, end, skew / setSkewForCentre) — the same
// source as createParameterLayout, built on the message thread by the processor.
// base01 of a routed destination comes from the raw APVTS atomic read at the
// tick (spawn-latched and tick destinations) or from the destination's base
// ramp mapped back to raw (ramped destinations, R47 — see Engine.cpp).
// ============================================================================
namespace ogran
{
    namespace ModDest
    {
        enum : int
        {
            None = 0, InputGain = 1, Mix = 2, OutputGain = 3, BufferLength = 4,
            StreamBase = 5, StreamStride = 24,
            Feedback = 101, FbLowCut = 102, FbHighCut = 103, FbSat = 104, FbDiff = 105,
            RevMix = 106, RevSize = 107, RevDamp = 108, LfoRate1 = 109
        };
        /** Per-stream field order (GranulatorParamIds::kModDestIds 5…28). */
        enum Field : int
        {
            Level = 0, Pan, Position, Spray, Size, SizeRand, Density, Jitter, Pitch, PitchRand, ReverseProb,
            Window, WindowSkew, StereoSpread, GainRand, FilterCutoff, FilterRand, FilterRes, Drive, Crush,
            CrushProb, RingFreq, RingRand, RingMix, NumFields
        };
        static_assert (static_cast<int> (NumFields) == static_cast<int> (StreamStride), "24 per-stream destinations");
        inline constexpr int stream (int s, int field) noexcept { return StreamBase + StreamStride * s + field; }
        static_assert (stream (3, RingMix) == 100 && Feedback == 101 && LfoRate1 + 3 == kModDests - 1, "destination table layout");
    }

    //==========================================================================
    // Engine ↔ raw maps (R47 table). raw is the APVTS value (dB, %, Hz, …).
    namespace ModMap
    {
        inline float gainToRaw (float g) noexcept        { return g > 0.0f ? 20.0f * std::log10 (g) : -24.0f; }
        inline float levelToRaw (float g) noexcept       { return g > 0.0f ? 20.0f * std::log10 (g) : -60.0f; }
        inline float fbKnobToRaw (float k) noexcept      { return k / kFbKnob * 100.0f; }
        inline float fbKnobFromRaw (float r) noexcept    { return kFbKnob * (std::clamp (r, 0.0f, 100.0f) / 100.0f); }
        inline float satDToRaw (float d) noexcept        { return (d - 1.0f) / kSatSpan * 100.0f; }
        inline float satDFromRaw (float r) noexcept      { return 1.0f + kSatSpan * (std::clamp (r, 0.0f, 100.0f) / 100.0f); }
        inline float pctFromRaw (float r) noexcept       { return std::clamp (r, 0.0f, 100.0f) / 100.0f; }

        /** raw → the GridParams::Stream field, exactly as GridParams::latch converts. */
        inline void streamFieldFromRaw (GridParams::Stream& st, int field, float r) noexcept
        {
            using namespace ModDest;
            switch (field)
            {
                case Level:        st.levelDb        = std::clamp (r, -60.0f, 6.0f); break;
                case Pan:          st.pan            = std::clamp (r, -100.0f, 100.0f) / 100.0f; break;
                case Position:     st.position       = pctFromRaw (r); break;
                case Spray:        st.spray          = pctFromRaw (r); break;
                case Size:         st.sizeMs         = std::clamp (r, 5.0f, 2000.0f); break;
                case SizeRand:     st.sizeRand       = pctFromRaw (r); break;
                case Density:      st.density        = std::clamp (r, 0.5f, 200.0f); break;
                case Jitter:       st.jitter         = pctFromRaw (r); break;
                case Pitch:        st.pitch          = std::clamp (r, -24.0f, 24.0f); break;
                case PitchRand:    st.pitchRand      = std::clamp (r, 0.0f, 24.0f); break;
                case ReverseProb:  st.reverseProb    = pctFromRaw (r); break;
                case Window:       st.window         = std::clamp (r, 0.0f, 1.0f); break;
                case WindowSkew:   st.windowSkew     = std::clamp (r, -100.0f, 100.0f) / 100.0f; break;
                case StereoSpread: st.spread         = pctFromRaw (r); break;
                case GainRand:     st.gainRand       = pctFromRaw (r); break;
                case FilterCutoff: st.filterCutoffHz = std::clamp (r, 20.0f, 20000.0f); break;
                case FilterRand:   st.filterRand     = pctFromRaw (r); break;
                case FilterRes:    st.filterRes      = pctFromRaw (r); break;
                case Drive:        st.drive          = pctFromRaw (r); break;
                case Crush:        st.crush          = pctFromRaw (r); break;
                case CrushProb:    st.crushProb      = pctFromRaw (r); break;
                case RingFreq:     st.ringFreqHz     = std::clamp (r, 20.0f, 5000.0f); break;
                case RingRand:     st.ringRand       = pctFromRaw (r); break;
                case RingMix:      st.ringMix        = pctFromRaw (r); break;
                default: break;
            }
        }
    }

    namespace ModMap
    {
        /** The engine value of a per-stream field (the P56 latched trace). */
        inline float streamFieldValue (const GridParams::Stream& st, int field) noexcept
        {
            using namespace ModDest;
            switch (field)
            {
                case Level: return st.levelDb;            case Pan: return st.pan;
                case Position: return st.position;        case Spray: return st.spray;
                case Size: return st.sizeMs;              case SizeRand: return st.sizeRand;
                case Density: return st.density;          case Jitter: return st.jitter;
                case Pitch: return st.pitch;              case PitchRand: return st.pitchRand;
                case ReverseProb: return st.reverseProb;  case Window: return st.window;
                case WindowSkew: return st.windowSkew;    case StereoSpread: return st.spread;
                case GainRand: return st.gainRand;        case FilterCutoff: return st.filterCutoffHz;
                case FilterRand: return st.filterRand;    case FilterRes: return st.filterRes;
                case Drive: return st.drive;              case Crush: return st.crush;
                case CrushProb: return st.crushProb;      case RingFreq: return st.ringFreqHz;
                case RingRand: return st.ringRand;        case RingMix: return st.ringMix;
                default: return 0.0f;
            }
        }
    }

    //==========================================================================
    class ModMatrix
    {
    public:
        // ── message thread ──────────────────────────────────────────────────
        void setRange (int dest, const juce::NormalisableRange<float>& r) noexcept
        {
            if (dest > 0 && dest < kModDests) ranges[static_cast<size_t> (dest)] = r;
        }
        void setRawPointer (int dest, std::atomic<float>* p) noexcept
        {
            if (dest > 0 && dest < kModDests) rawPtr[static_cast<size_t> (dest)] = p;
        }
        const juce::NormalisableRange<float>& range (int dest) const noexcept { return ranges[static_cast<size_t> (dest)]; }

        /** prepare: slots snap to the latched routes (no fade at load), residuals 0. */
        void prepare (int rampCellsIn, const GridParams& gp) noexcept
        {
            rampCells = rampCellsIn;
            for (int m = 0; m < kModSlots; ++m)
            {
                auto& s = slots[static_cast<size_t> (m)];
                s.src  = gp.mod[m].src;
                s.dest = gp.mod[m].dest;
                s.depth.reset (gp.mod[m].depth);
                s.lastContrib = 0.0f;
            }
            for (auto& r : residual) r.reset (0.0f);
            residualLive.fill (false);
            offPrev.fill (0.0f);
            offCur.fill (0.0f);
            lastTelRaw.fill (std::numeric_limits<float>::quiet_NaN());
            routedNow = 0;
        }

        // ── audio thread, grid tick ─────────────────────────────────────────
        /** Returns the number of route changes seen this tick. */
        int tick (const GridParams& gp, const SourceValues& src) noexcept
        {
            int changes = 0;
            offPrev = offCur;
            offCur.fill (0.0f);

            for (int m = 0; m < kModSlots; ++m)
            {
                auto& s = slots[static_cast<size_t> (m)];
                const auto& want = gp.mod[m];
                if (want.src != s.src || want.dest != s.dest)
                {
                    ++changes;
                    const bool wasLive = s.src != ModSrc::None && s.dest != ModDest::None;
                    if (wasLive && ! noResidual && ! juce::exactlyEqual (s.lastContrib, 0.0f))
                    {
                        auto& r = residual[static_cast<size_t> (s.dest)];
                        r.reset (r.current() + s.lastContrib);
                        r.setTarget (0.0f, rampCells);
                        residualLive[static_cast<size_t> (s.dest)] = true;
                    }
                    s.src  = want.src;
                    s.dest = want.dest;
                    s.depth.reset (0.0f);
                }
                s.depth.setTarget (want.depth, rampCells);
                s.depth.tick();

                const bool live = s.src != ModSrc::None && s.dest != ModDest::None;
                s.lastContrib = live ? s.depth.current() * src.v[s.src] : 0.0f;
                if (live)
                    offCur[static_cast<size_t> (s.dest)] += s.lastContrib;
            }

            for (int d = 1; d < kModDests; ++d)
            {
                if (! residualLive[static_cast<size_t> (d)])
                    continue;
                auto& r = residual[static_cast<size_t> (d)];
                r.tick();
                offCur[static_cast<size_t> (d)] += r.current();
                if (r.settledAt (0.0f))
                    residualLive[static_cast<size_t> (d)] = false;
            }

            routedNow = 0;
            for (int d = 1; d < kModDests; ++d)
            {
                float& o = offCur[static_cast<size_t> (d)];
                if (! std::isfinite (o)) o = 0.0f;
                if (o > 0.0f || o < 0.0f) ++routedNow;
                else o = 0.0f;   // canonical +0 (−0 counts as unrouted)
            }
            return changes;
        }

        bool  routed (int d) const noexcept      { return fullRoundTrip || offCur[static_cast<size_t> (d)] > 0.0f || offCur[static_cast<size_t> (d)] < 0.0f; }
        bool  routedPrev (int d) const noexcept  { return fullRoundTrip || offPrev[static_cast<size_t> (d)] > 0.0f || offPrev[static_cast<size_t> (d)] < 0.0f; }
        float offset (int d) const noexcept      { return offCur[static_cast<size_t> (d)]; }
        float offsetPrev (int d) const noexcept  { return offPrev[static_cast<size_t> (d)]; }
        int   routedCount() const noexcept       { return routedNow; }

        /** Offset at piece offset o of the cell (spawn-latched, R48). */
        float offsetAt (int d, double o) const noexcept
        {
            const float p = offPrev[static_cast<size_t> (d)], c = offCur[static_cast<size_t> (d)];
            return p + (c - p) * static_cast<float> (o / static_cast<double> (kCell));
        }

        /** The latched raw APVTS value of a destination, clamped to its range. */
        float rawNow (int d) const noexcept
        {
            const auto* p = rawPtr[static_cast<size_t> (d)];
            const auto& r = ranges[static_cast<size_t> (d)];
            if (p == nullptr) return r.start;
            const float v = p->load (std::memory_order_relaxed);
            return std::isfinite (v) ? std::clamp (v, r.start, r.end) : r.start;
        }

        /** convertFrom0to1 (clamp (convertTo0to1 (raw) + off, 0, 1)). */
        float apply (int d, float raw, float off) const noexcept
        {
            const auto& r = ranges[static_cast<size_t> (d)];
            const float v01 = std::clamp (r.convertTo0to1 (std::clamp (raw, r.start, r.end)) + off, 0.0f, 1.0f);
            return r.convertFrom0to1 (v01);
        }
        float v01 (int d, float raw, float off) const noexcept
        {
            const auto& r = ranges[static_cast<size_t> (d)];
            return std::clamp (r.convertTo0to1 (std::clamp (raw, r.start, r.end)) + off, 0.0f, 1.0f);
        }

        /** Telemetry (P55): routed → v01 at the knob, unrouted → base01 when the raw moved. */
        template <size_t N>
        void writeTelemetry (std::array<std::atomic<float>, N>& out) noexcept
        {
            static_assert (N == static_cast<size_t> (kModDests), "one atomic per destination");
            for (int d = 1; d < kModDests; ++d)
            {
                const float raw = rawNow (d);
                if (routed (d))
                {
                    out[static_cast<size_t> (d)].store (v01 (d, raw, offset (d)), std::memory_order_relaxed);
                    lastTelRaw[static_cast<size_t> (d)] = std::numeric_limits<float>::quiet_NaN();   // re-arm the unrouted write
                }
                else if (! juce::exactlyEqual (raw, lastTelRaw[static_cast<size_t> (d)]))
                {
                    lastTelRaw[static_cast<size_t> (d)] = raw;
                    out[static_cast<size_t> (d)].store (ranges[static_cast<size_t> (d)].convertTo0to1 (raw), std::memory_order_relaxed);
                }
            }
        }

        // Test hooks (Engine copies TestHooks).
        bool noResidual    = false;
        bool fullRoundTrip = false;

        float residualValue (int d) const noexcept { return residual[static_cast<size_t> (d)].current(); }

    private:
        struct Slot
        {
            int src = 0, dest = 0;
            ExactRamp depth;
            float lastContrib = 0.0f;   // depth·src of the latest evaluated cell
        };
        std::array<Slot, kModSlots> slots {};
        std::array<ExactRamp, kModDests> residual {};
        std::array<bool, kModDests> residualLive {};
        std::array<float, kModDests> offPrev {}, offCur {};
        std::array<float, kModDests> lastTelRaw {};
        std::array<juce::NormalisableRange<float>, kModDests> ranges {};
        std::array<std::atomic<float>*, kModDests> rawPtr {};
        int rampCells = 30;
        int routedNow = 0;
    };
} // namespace ogran
