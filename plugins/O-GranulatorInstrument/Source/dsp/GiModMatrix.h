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
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <limits>
#include "forked/ControlGrid.h"
#include "forked/EngineConstants.h"

// ============================================================================
// GiModMatrix — Phase 2.5a (PLAN-2.5a T6, PD3; RESEARCH-2.5a §2.2).
//
// A line-for-line PORT of the parent ModMatrix (82406fce
// plugins/O-Granulator/Source/dsp/ModMatrix.h, kept VERBATIM and uncompiled
// in forked/ModMatrix.h) sized to the instrument's frozen tables:
// 150 destinations (GiParamIds::kModDestIds) × 18 sources (mod-sources.tsv).
//
// Per grid tick (statement order of the parent ModMatrix::tick):
//   1. slot compare: source / dest changed → if the old route was live on
//      the GLOBAL path, its frozen contribution is added to residual[oldDest]
//      (reset (current + lastContrib)), which ramps linearly to exactly 0 over
//      rampCells; the slot depth restarts at 0 and ramps to its target.
//   2. depth setTarget + tick; off[d] = Σ depth·src + residual[d], summed
//      BEFORE the single clamp.
//   3. non-finite → 0; canonical +0 (a −0 sum counts as unrouted).
//   apply (d, raw, off) = convertFrom0to1 (clamp (convertTo0to1 (clamp (raw)) + off, 0, 1)).
//
// Instrument extension (PD4 / PD5): a slot whose source is per-voice and whose
// destination is per-voice (class V) runs on the PER-VOICE path; the engine
// evaluates it per voice (GiVoiceMod). Such a slot contributes nothing to the
// global sum here, and its route-change residuals are per voice (the engine
// reads changedFrom()).
// Only touched destinations are rolled (≤ 16 slot dests + live residuals).
// ============================================================================
namespace ogi
{
    /** The 18-entry source table (tests/golden/mod-sources.tsv, frozen). */
    namespace GiSrc
    {
        enum : int
        {
            None = 0, Lfo1 = 1, Lfo4 = 4, ModEnv1 = 5, ModEnv2 = 6, Velocity = 7, Key = 8, NoteRandom = 9,
            ModWheel = 10, Pressure = 11, Slide = 12, Bend = 13, Macro1 = 14, Macro4 = 17, Count = 18
        };
        static_assert (Count == kGiModSources, "18 sources incl. None");
        inline constexpr bool isLfo (int s) noexcept       { return s >= Lfo1 && s <= Lfo4; }
        inline constexpr bool isMacro (int s) noexcept     { return s >= Macro1 && s <= Macro4; }
        /** Per-voice class (mod-sources.tsv "per-voice"). */
        inline constexpr bool isPerVoice (int s) noexcept  { return (s >= ModEnv1 && s <= NoteRandom) || (s >= Pressure && s <= Bend); }
    }

    /** The 150-entry destination table (GiParamIds::kModDestIds, APPEND-ONLY;
        table version 2, PLAN-R1 PD14). */
    namespace GiDest
    {
        enum : int
        {
            None = 0, OutputGain = 1, StreamBase = 2, StreamStride = 36, LfoRate1 = 146
        };
        /** Per-stream field order. Fields 0...27 are the VOICE fields (the
            parent's 24, then scan_rate, loop_start, loop_end, key_track):
            NumFields stays their count, so every per-voice loop is bounded by
            it. Fields 28...35 are the stream's feedback and reverb (R1): one
            value per stream, never per voice (class G). */
        enum Field : int
        {
            Level = 0, Pan, Position, Spray, Size, SizeRand, Density, Jitter, Pitch, PitchRand, ReverseProb,
            Window, WindowSkew, StereoSpread, GainRand, FilterCutoff, FilterRand, FilterRes, Drive, Crush,
            CrushProb, RingFreq, RingRand, RingMix, ScanRate, LoopStart, LoopEnd, KeyTrack, NumFields,
            Feedback = NumFields, FbLowCut, FbHighCut, FbSat, FbDiff, RevMix, RevSize, RevDamp, NumStreamDests
        };
        static_assert (static_cast<int> (NumFields) == 28, "28 per-voice stream destinations");
        static_assert (static_cast<int> (NumStreamDests) == static_cast<int> (StreamStride), "36 per-stream destinations (28 voice + 8 stream-global)");
        inline constexpr int stream (int s, int field) noexcept { return StreamBase + StreamStride * s + field; }
        static_assert (stream (3, KeyTrack) == 137 && stream (0, 28) == 30 && stream (3, RevDamp) == 145
                       && LfoRate1 == 146 && LfoRate1 + 3 == kGiModDests - 1, "destination table layout");
        inline constexpr bool isStream (int d) noexcept    { return d >= StreamBase && d < LfoRate1; }
        inline constexpr int  streamOf (int d) noexcept    { return (d - StreamBase) / StreamStride; }
        inline constexpr int  fieldOf (int d) noexcept     { return (d - StreamBase) % StreamStride; }
        inline constexpr bool isLfoRate (int d) noexcept   { return d >= LfoRate1 && d < LfoRate1 + kGiLfos; }
        /** Class V (mod-dests.tsv): the 28 voice fields of each stream. The 8
            feedback / reverb fields share the stream block but are class G —
            a per-voice source reaches them through the global path (newest
            voice, D4). G/V: the LFO rates. */
        inline constexpr bool isVoiceClass (int d) noexcept { return isStream (d) && fieldOf (d) < NumFields; }
        static_assert (isVoiceClass (stream (3, KeyTrack)) && ! isVoiceClass (stream (0, Feedback)) && ! isVoiceClass (stream (3, RevDamp)),
                       "the stream-global fields are not per-voice");
    }

    //==========================================================================
    // Engine ↔ raw maps (parent ModMap, R47). raw is the APVTS value.
    namespace GiModMap
    {
        inline float gainToRaw (float g) noexcept   { return g > 0.0f ? 20.0f * std::log10 (g) : -24.0f; }
        inline float levelToRaw (float g) noexcept  { return g > 0.0f ? 20.0f * std::log10 (g) : -60.0f; }
        inline float pctFromRaw (float r) noexcept  { return std::clamp (r, 0.0f, 100.0f) / 100.0f; }
        inline float satDToRaw (float d) noexcept   { return (d - 1.0f) / kSatSpan * 100.0f; }
        inline float satDFromRaw (float r) noexcept { return 1.0f + kSatSpan * (std::clamp (r, 0.0f, 100.0f) / 100.0f); }

        /** raw → the GiGridParams::Stream field, exactly as GiGridParams::latch converts. */
        inline void streamFieldFromRaw (GiGridParams::Stream& st, int field, float r) noexcept
        {
            using namespace GiDest;
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
                case Pitch:        st.pitchSt        = std::clamp (r, -24.0f, 24.0f); break;
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
                case ScanRate:     st.scanRate       = std::clamp (r, -400.0f, 400.0f) / 100.0f; break;
                case LoopStart:    st.loopStart      = pctFromRaw (r); break;
                case LoopEnd:      st.loopEnd        = pctFromRaw (r); break;
                case KeyTrack:     st.keyTrack       = pctFromRaw (r); break;
                default: break;
            }
        }

        /** The engine value of a per-stream field (the latched trace). */
        inline float streamFieldValue (const GiGridParams::Stream& st, int field) noexcept
        {
            using namespace GiDest;
            switch (field)
            {
                case Level: return st.levelDb;            case Pan: return st.pan;
                case Position: return st.position;        case Spray: return st.spray;
                case Size: return st.sizeMs;              case SizeRand: return st.sizeRand;
                case Density: return st.density;          case Jitter: return st.jitter;
                case Pitch: return st.pitchSt;            case PitchRand: return st.pitchRand;
                case ReverseProb: return st.reverseProb;  case Window: return st.window;
                case WindowSkew: return st.windowSkew;    case StereoSpread: return st.spread;
                case GainRand: return st.gainRand;        case FilterCutoff: return st.filterCutoffHz;
                case FilterRand: return st.filterRand;    case FilterRes: return st.filterRes;
                case Drive: return st.drive;              case Crush: return st.crush;
                case CrushProb: return st.crushProb;      case RingFreq: return st.ringFreqHz;
                case RingRand: return st.ringRand;        case RingMix: return st.ringMix;
                case ScanRate: return st.scanRate;        case LoopStart: return st.loopStart;
                case LoopEnd: return st.loopEnd;          case KeyTrack: return st.keyTrack;
                default: return 0.0f;
            }
        }
    }

    //==========================================================================
    class GiModMatrix
    {
    public:
        static constexpr int kDests = kGiModDests, kSources = kGiModSources, kSlots = kGiModSlots;

        // ── message thread ──────────────────────────────────────────────────
        void setRange (int dest, const juce::NormalisableRange<float>& r) noexcept
        {
            if (dest > 0 && dest < kDests) ranges[static_cast<size_t> (dest)] = r;
        }
        void setRawPointer (int dest, std::atomic<float>* p) noexcept
        {
            if (dest > 0 && dest < kDests) rawPtr[static_cast<size_t> (dest)] = p;
        }
        const juce::NormalisableRange<float>& range (int dest) const noexcept { return ranges[static_cast<size_t> (dest)]; }
        bool hasRange (int dest) const noexcept { return dest > 0 && dest < kDests && rawPtr[static_cast<size_t> (dest)] != nullptr; }

        /** prepare: slots snap to the latched routes (no fade at load), residuals 0. */
        void prepare (int rampCellsIn, const GiGridParams& gp) noexcept
        {
            rampCells = rampCellsIn;
            for (int m = 0; m < kSlots; ++m)
            {
                auto& s = slots[static_cast<size_t> (m)];
                s.src  = gp.mod[m].src;
                s.dest = gp.mod[m].dest;
                s.depth.reset (gp.mod[m].depth);
                s.lastContrib = 0.0f;
                s.pv = false;
                s.changed = false;
                s.oldSrc = s.oldDest = 0;
                s.oldPv = false;
            }
            for (auto& r : residual) r.reset (0.0f);
            residualLive.fill (false);
            offPrev.fill (0.0f);
            offCur.fill (0.0f);
            lastTelRaw.fill (std::numeric_limits<float>::quiet_NaN());
            routedNow = 0;
            numTouched = numTouchedPrev = 0;
            touchedMark.fill (false);
        }

        // ── audio thread, grid tick ─────────────────────────────────────────
        /** src: the GLOBAL-path value of every source this cell (global sources,
            global LFOs, newest-voice mono values of per-voice sources).
            pvPath[m]: slot m runs on the per-voice path this cell (live, per-
            voice source, class-V destination); its contribution is NOT summed
            here. Returns the number of route changes seen this tick. */
        int tick (const GiGridParams& gp, const float* src, const bool* pvPath) noexcept
        {
            int changes = 0;
            // offPrev = offCur; offCur = 0 — over the touched set only (the
            // untouched entries are +0 in both arrays by construction).
            for (int i = 0; i < numTouchedPrev; ++i)
                offPrev[static_cast<size_t> (touchedPrev[i])] = 0.0f;
            for (int i = 0; i < numTouched; ++i)
            {
                const int d = touched[i];
                offPrev[static_cast<size_t> (d)] = offCur[static_cast<size_t> (d)];
                offCur[static_cast<size_t> (d)] = 0.0f;
                touchedMark[static_cast<size_t> (d)] = false;
            }
            std::copy (touched, touched + numTouched, touchedPrev);
            numTouchedPrev = numTouched;
            numTouched = 0;

            for (int m = 0; m < kSlots; ++m)
            {
                auto& s = slots[static_cast<size_t> (m)];
                const auto& want = gp.mod[m];
                s.changed = false;
                if (want.src != s.src || want.dest != s.dest)
                {
                    ++changes;
                    const bool wasLive = s.src != GiSrc::None && s.dest != GiDest::None;
                    s.changed = wasLive;
                    s.oldSrc = s.src;
                    s.oldDest = s.dest;
                    s.oldPv = s.pv;
                    if (wasLive && ! s.pv && ! noResidual && ! juce::exactlyEqual (s.lastContrib, 0.0f))
                    {
                        auto& r = residual[static_cast<size_t> (s.dest)];
                        r.reset (r.current() + s.lastContrib);
                        r.setTarget (0.0f, rampCells);
                        residualLive[static_cast<size_t> (s.dest)] = true;
                    }
                    s.src  = want.src;
                    s.dest = want.dest;
                    if (noDepthRamp)
                        s.depth.reset (want.depth);   // control: the new route enters at full depth
                    else
                        s.depth.reset (0.0f);
                }
                if (noDepthRamp)
                {
                    s.depth.reset (want.depth);       // … and a depth edit snaps
                }
                else
                {
                    s.depth.setTarget (want.depth, rampCells);
                    s.depth.tick();
                }

                const bool live = s.src != GiSrc::None && s.dest != GiDest::None;
                s.pv = live && pvPath[m];
                s.lastContrib = (live && ! s.pv) ? s.depth.current() * src[s.src] : 0.0f;
                if (live && ! s.pv)
                {
                    touch (s.dest);
                    offCur[static_cast<size_t> (s.dest)] += s.lastContrib;
                }
            }

            for (int d = 1; d < kDests; ++d)
            {
                if (! residualLive[static_cast<size_t> (d)])
                    continue;
                auto& r = residual[static_cast<size_t> (d)];
                r.tick();
                touch (d);
                offCur[static_cast<size_t> (d)] += r.current();
                if (r.settledAt (0.0f))
                    residualLive[static_cast<size_t> (d)] = false;
            }

            routedNow = 0;
            for (int i = 0; i < numTouched; ++i)
            {
                float& o = offCur[static_cast<size_t> (touched[i])];
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
        float base01 (int d, float raw) const noexcept
        {
            const auto& r = ranges[static_cast<size_t> (d)];
            return r.convertTo0to1 (std::clamp (raw, r.start, r.end));
        }

        /** Telemetry (parent P55): routed → v01 at the knob, unrouted → base01
            when the raw moved. `voiceOff` (may be null): the newest voice's
            per-voice offsets, added for V destinations (PD16). */
        void writeTelemetry (std::atomic<float>* out, const float* voiceOff) noexcept
        {
            for (int d = 1; d < kDests; ++d)
            {
                const float raw = rawNow (d);
                const float ov = voiceOff != nullptr ? voiceOff[d] : 0.0f;
                const float off = offset (d) + ov;
                if (routed (d) || ov > 0.0f || ov < 0.0f)
                {
                    out[d].store (v01 (d, raw, off), std::memory_order_relaxed);
                    lastTelRaw[static_cast<size_t> (d)] = std::numeric_limits<float>::quiet_NaN();   // re-arm the unrouted write
                }
                else if (! juce::exactlyEqual (raw, lastTelRaw[static_cast<size_t> (d)]))
                {
                    lastTelRaw[static_cast<size_t> (d)] = raw;
                    out[d].store (ranges[static_cast<size_t> (d)].convertTo0to1 (raw), std::memory_order_relaxed);
                }
            }
        }

        // ── slot views for the per-voice pass ──────────────────────────────
        struct SlotView { int src = 0, dest = 0; float depth = 0.0f; bool pv = false; bool changed = false; int oldSrc = 0, oldDest = 0; bool oldPv = false; };
        SlotView slot (int m) const noexcept
        {
            const auto& s = slots[static_cast<size_t> (m)];
            return { s.src, s.dest, s.depth.current(), s.pv, s.changed, s.oldSrc, s.oldDest, s.oldPv };
        }
        int currentSrc (int m) const noexcept  { return slots[static_cast<size_t> (m)].src; }
        int currentDest (int m) const noexcept { return slots[static_cast<size_t> (m)].dest; }
        int  getRampCells() const noexcept     { return rampCells; }

        // Test hooks (Engine copies EngineTestHooks; parent names).
        bool noResidual    = false;
        bool fullRoundTrip = false;
        bool noDepthRamp   = false;

        float residualValue (int d) const noexcept { return residual[static_cast<size_t> (d)].current(); }

    private:
        void touch (int d) noexcept
        {
            if (touchedMark[static_cast<size_t> (d)])
                return;
            touchedMark[static_cast<size_t> (d)] = true;
            touched[numTouched++] = d;
        }

        struct Slot
        {
            int src = 0, dest = 0;
            ExactRamp depth;
            float lastContrib = 0.0f;   // depth·src of the latest evaluated cell (global path)
            bool pv = false;            // on the per-voice path this cell
            bool changed = false;       // a live route changed source / dest at this tick
            int oldSrc = 0, oldDest = 0;
            bool oldPv = false;
        };
        std::array<Slot, kSlots> slots {};
        std::array<ExactRamp, kDests> residual {};
        std::array<bool, kDests> residualLive {};
        std::array<float, kDests> offPrev {}, offCur {};
        std::array<float, kDests> lastTelRaw {};
        std::array<juce::NormalisableRange<float>, kDests> ranges {};
        std::array<std::atomic<float>*, kDests> rawPtr {};
        std::array<bool, kDests> touchedMark {};
        int touched[kDests] {}, touchedPrev[kDests] {};
        int numTouched = 0, numTouchedPrev = 0;
        int rampCells = 30;
        int routedNow = 0;
    };
} // namespace ogi
