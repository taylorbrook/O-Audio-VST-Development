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

//==============================================================================
// Stage 3 / Phase 3.3 rows (PLAN-3.3 P128, P133).
//
//   UX1a–e  the page's route-edit write sequences (clear, drop, dest swap,
//           source swap, depth drag), multi-write sequences spread over 0 / 1 /
//           7 cells, on s1_level / mix / output_gain / feedback, LFO1 2 Hz and
//           Onset: (a) |dv01| per cell <= 1.05x the analytic bound (the MX3
//           form, generalised to a sequence of edits), (b) max curvature
//           <= 2.0x the held pre / post-state references (the MF8 form)
//   UX1nR   noResidual on clear: > 2x the bound on >= 1 destination
//   UX1nD   noDepthRamp on drop: > 2x the bound on >= 1 destination
//==============================================================================
namespace { namespace eg33
{
    using namespace eg;
    using eg25b::operator+;
    using eg25b::route;
    using eg22::sp;
    namespace D = ogran::ModDest;
    namespace S = ogran::ModSrc;
    using Hooks = std::function<void (ogran::Engine::TestHooks&)>;

    //==========================================================================
    // UX1 — route-edit write sequences (P128, R127)
    //==========================================================================
    struct UxDest { int d; const char* name; };
    inline std::vector<UxDest> uxDests()
    {
        return { { D::stream (0, D::Level), "s1_level" }, { D::Mix, "mix" }, { D::OutputGain, "output_gain" }, { D::Feedback, "feedback" } };
    }

    /** One timed write sequence: (cell offset from the edit cell, param suffix, value). */
    struct UxWrite { int cell; const char* field; float v; };
    /** An edit for the bound: the cell it lands in and K (the |depth| it ramps in plus
        the |depth| of the contribution it moves to a residual), in v01 units. */
    struct UxEdit { int cell; double K; bool swap; };

    struct UxTrace
    {
        std::vector<std::array<float, 2>> dest;     // per cell: the (up to) two destinations under test
        std::vector<std::array<float, 3>> src;      // per cell: LFO1, LFO2, Onset
        std::array<float, ogran::kModDests> prev {};
        int crossTalk = 0;
        bool first = true;
    };

    constexpr juce::int64 kAtLfo = 102016, kAtOnset = 99008;   // MF8: LFO1 2 Hz at its peak; Onset >= 30 ms from every swell

    /** The MF8 base at block 32 (one trace entry per cell): sine 440 (LFO) or the swell
        carrier (Onset), 4 streams, mix 50 %, feedback 50 %, LFO1 2 Hz sine, LFO2 3 Hz sine. */
    inline Job uxJob (bool onset, const P& routes, std::vector<Event> ev, int d0, int d1,
                      std::shared_ptr<UxTrace> tr, Hooks hk = {})
    {
        Job j;
        j.fs = 48000.0; j.len = 3 * 48000; j.block = 32; j.prepBlock = 32;
        j.params = eg22::base ({ 1, 2, 3, 4 }) + P { { "mix", 50.0f }, { "feedback", 50.0f }, { "reverb_mix", 0.0f }, { "lfo1_shape", 0.0f },
                                                    { "lfo1_rate", 2.0f }, { "lfo2_shape", 0.0f }, { "lfo2_rate", 3.0f }, { "onset_threshold", -50.0f } };
        for (int s = 1; s <= 4; ++s)
            j.params = j.params + P { { sp (s, "density"), 20.0f }, { sp (s, "size"), 200.0f }, { sp (s, "spray"), 0.0f }, { sp (s, "jitter"), 20.0f },
                                      { sp (s, "window"), 0.0f }, { sp (s, "window_skew"), 0.0f } };
        j.params = j.params + routes;
        j.stim = onset ? eg26b::swell (3.0, 0.001) : eg26b::sine440();
        j.events = std::move (ev);
        if (hk) j.pre = [hk] (Proc& p) { hk (p.getEngineForTest().hooks); };
        if (tr)
            j.after = [tr, d0, d1] (Proc& p, juce::int64)
            {
                const auto& e = p.getEngineForTest();
                std::array<float, ogran::kModDests> now {};
                for (int k = 1; k < ogran::kModDests; ++k) now[(size_t) k] = e.getDestTelemetry (k);
                if (! tr->first)
                    for (int k = 1; k < ogran::kModDests; ++k)
                        if (k != d0 && k != d1 && now[(size_t) k] != tr->prev[(size_t) k]) ++tr->crossTalk;
                tr->first = false;
                tr->prev = now;
                tr->dest.push_back ({ now[(size_t) d0], d1 > 0 ? now[(size_t) d1] : 0.0f });
                tr->src.push_back ({ e.getSrcTelemetry (S::Lfo1), e.getSrcTelemetry (2), e.getSrcTelemetry (S::Onset) });
            };
        return j;
    }

    inline std::vector<Event> uxEvents (juce::int64 at, const std::vector<UxWrite>& ws)
    {
        // group writes landing in the same cell into one event (the page's same-tick case)
        std::vector<Event> ev;
        std::vector<UxWrite> sorted = ws;
        std::stable_sort (sorted.begin(), sorted.end(), [] (const UxWrite& a, const UxWrite& b) { return a.cell < b.cell; });
        for (size_t i = 0; i < sorted.size();)
        {
            std::vector<UxWrite> grp;
            const int c = sorted[i].cell;
            while (i < sorted.size() && sorted[i].cell == c) grp.push_back (sorted[i++]);
            ev.push_back ({ at + (juce::int64) c * ogran::kCell, [grp] (Proc& p)
            {
                for (const auto& w : grp) eg::setP (p, juce::String ("mod1_") + w.field, w.v);
            } });
        }
        return ev;
    }

    struct UxVerdict { double worst = 0.0; int violations = 0; double dmax = 0.0; };
    /** |dv01| per cell vs 1.05 x B: B(t) = slots · 0.6 · max|dsrc| over t−1…t+1 (the involved
        sources) + Σ K_e / rampCells over the edits whose window [e, e + rampCells + 2] holds t.
        slots = 2 inside a swap window (the residual's old route and the new route can share a dest). */
    inline UxVerdict uxCheck (const UxTrace& tr, int lane, const std::vector<int>& srcs, int eCell0,
                              const std::vector<UxEdit>& edits, int rampCells)
    {
        UxVerdict v;
        const size_t n = tr.dest.size();
        auto dsrc = [&] (int s, size_t t) { return t >= 1 && t < n ? std::abs ((double) tr.src[t][(size_t) s] - (double) tr.src[t - 1][(size_t) s]) : 0.0; };
        for (size_t t = 1; t < n; ++t)
        {
            const double dv = std::abs ((double) tr.dest[t][(size_t) lane] - (double) tr.dest[t - 1][(size_t) lane]);
            double ms = 0.0;
            for (size_t u = t - 1; u <= t + 1; ++u)
                for (int s : srcs) ms = juce::jmax (ms, dsrc (s, u));
            double ramp = 0.0; bool swapWin = false;
            for (const auto& e : edits)
            {
                const int c = eCell0 + e.cell;
                if ((int) t >= c && (int) t <= c + rampCells + 2) { ramp += e.K / (double) rampCells; swapWin = swapWin || e.swap; }
            }
            const double B = (swapWin ? 2.0 : 1.0) * 0.6 * ms + ramp;
            v.dmax = juce::jmax (v.dmax, dv);
            if (dv > 1.05 * B + 1e-6) ++v.violations;
            if (dv > 2e-6) v.worst = juce::jmax (v.worst, B > 1e-9 ? dv / B : 1e9);
        }
        return v;
    }

    enum UxSeq { kClear0, kClear1, kClear7, kDrop0, kDrop1, kDrop7, kDestSwap, kSrcSwap, kDrag, kNumSeq };
    inline const char* seqName (int q)
    {
        static const char* n[] { "clear (same cell)", "clear (1 cell)", "clear (7 cells)", "drop (same cell)", "drop (1 cell)", "drop (7 cells)",
                                 "dest swap", "source swap", "depth drag" };
        return n[q];
    }
    inline int seqSpread (int q) { return q == kClear1 || q == kDrop1 ? 1 : (q == kClear7 || q == kDrop7 ? 7 : 0); }

    struct UxCase
    {
        P routes;
        std::vector<UxWrite> writes;
        std::vector<UxEdit> edits;
        int d1 = 0;               // second destination under test (dest swap), else 0
        int lastCell = 0;         // the last write's cell offset (κ window end)
    };

    /** `S1` the base source, `S2` its swap partner, `d` / `d2` the destinations. */
    inline UxCase uxCase (int q, int S1, int S2, int d, int d2)
    {
        UxCase c;
        const int s = seqSpread (q);
        switch (q)
        {
            case kClear0: case kClear1: case kClear7:
                // clearSlot: source -> 0, dest -> 0, depth -> 0 (the residual goes to the OLD dest)
                c.routes = route (1, S1, d, 50.0f);
                c.writes = { { 0, "source", 0.0f }, { s, "dest", 0.0f }, { 2 * s, "depth", 0.0f } };
                c.edits = { { 0, 0.5, false } };
                c.lastCell = 2 * s;
                break;
            case kDrop0: case kDrop1: case kDrop7:
                // addRoute: dest -> d, depth -> 0.5 default, source LAST (the slot is dead until it lands)
                c.routes = route (1, 0, 0, 0.0f);
                c.writes = { { 0, "dest", (float) d }, { s, "depth", 50.0f }, { 2 * s, "source", (float) S1 } };
                c.edits = { { 2 * s, 0.5, false } };
                c.lastCell = 2 * s;
                break;
            case kDestSwap:
                c.routes = route (1, S1, d, 50.0f);
                c.writes = { { 0, "dest", (float) d2 } };
                c.edits = { { 0, 1.0, true } };
                c.d1 = d2;
                break;
            case kSrcSwap:
                c.routes = route (1, S1, d, 50.0f);
                c.writes = { { 0, "source", (float) S2 } };
                c.edits = { { 0, 1.0, true } };
                break;
            case kDrag:
            default:
                // 10 retargets +60 -> -60 over 300 ms (a fader drag: 12 % every 45 cells = 30 ms)
                c.routes = route (1, S1, d, 60.0f);
                for (int i = 1; i <= 10; ++i)
                {
                    c.writes.push_back ({ 45 * (i - 1), "depth", 60.0f - 12.0f * (float) i });
                    c.edits.push_back ({ 45 * (i - 1), 0.12, false });
                }
                c.lastCell = 45 * 9;
                break;
        }
        return c;
    }

    void ux1Rows()
    {
        std::printf ("\n-- UX1. D36 route edits: the page's write sequences (clear / drop / swaps / depth drag) x 0 / 1 / 7-cell spreads (P128)\n");
        const auto ds = uxDests();
        int rampCells = 0;
        {
            auto o = eg25::renderAll ({ uxJob (false, {}, {}, ds[0].d, 0, nullptr) });
            rampCells = o[0].eng().getRampCells();
        }
        // Per (source kind, destination): the 9 subjects, then the references
        // { empty slot, held S1 @ 50, held S2 @ 50, held S1 @ +60, held S1 @ -60 }.
        constexpr size_t kSub = kNumSeq, kRef = 5, kPer = kSub + kRef;
        struct Meta { bool onset; size_t di; };
        std::vector<Job> jobs;
        std::vector<std::shared_ptr<UxTrace>> trs;
        std::vector<UxCase> cases;
        std::vector<Meta> meta;
        for (int sk = 0; sk < 2; ++sk)
        {
            const bool onset = sk == 1;
            const int S1 = onset ? (int) S::Onset : (int) S::Lfo1, S2 = onset ? (int) S::Lfo1 : 2;
            const juce::int64 at = onset ? kAtOnset : kAtLfo;
            for (size_t di = 0; di < ds.size(); ++di)
            {
                const int d = ds[di].d, d2 = ds[(di + 1) % ds.size()].d;
                meta.push_back ({ onset, di });
                for (int q = 0; q < kNumSeq; ++q)
                {
                    auto c = uxCase (q, S1, S2, d, d2);
                    auto tr = std::make_shared<UxTrace>();
                    jobs.push_back (uxJob (onset, c.routes, uxEvents (at, c.writes), d, c.d1, tr));
                    trs.push_back (tr);
                    cases.push_back (c);
                }
                for (const auto& r : { route (1, 0, 0, 0.0f), route (1, S1, d, 50.0f), route (1, S2, d, 50.0f), route (1, S1, d, 60.0f), route (1, S1, d, -60.0f) })
                {
                    jobs.push_back (uxJob (onset, r, {}, d, 0, nullptr));
                    trs.push_back (nullptr);
                    cases.push_back ({});
                }
            }
        }
        // the dest-swap post-state reference is the next destination's held S1 @ 50 (same source kind)
        auto o = eg25::renderAll (jobs);

        Tally tq[kNumSeq];
        double worstB[kNumSeq] {}, worstK[kNumSeq] {};
        int cross = 0;
        bool wetOk = true;
        double srcAtEdit[2] {};   // |base source| at the edit cell (non-vacuity: LFO1, Onset)
        for (size_t mi = 0; mi < meta.size(); ++mi)
        {
            const auto& m = meta[mi];
            const size_t base = mi * kPer;
            const size_t nextBase = (m.onset ? 4 : 0) * kPer + ((m.di + 1) % ds.size()) * kPer;
            const auto& rEmpty = o[base + kSub], &r50 = o[base + kSub + 1], &r50s2 = o[base + kSub + 2], &r60 = o[base + kSub + 3], &rm60 = o[base + kSub + 4];
            const auto& r50next = o[nextBase + kSub + 1];
            const juce::int64 at = m.onset ? kAtOnset : kAtLfo;
            const int eCell0 = (int) (at / ogran::kCell);
            const std::vector<int> srcs = m.onset ? std::vector<int> { 2, 0 } : std::vector<int> { 0, 1 };   // lanes: LFO1 0, LFO2 1, Onset 2
            for (int q = 0; q < kNumSeq; ++q)
            {
                const size_t i = base + (size_t) q;
                const auto& c = cases[i];
                const auto& tr = *trs[i];
                if (q == kClear0 && (size_t) eCell0 < tr.src.size())
                    srcAtEdit[m.onset ? 1 : 0] = juce::jmax (srcAtEdit[m.onset ? 1 : 0], (double) std::abs (tr.src[(size_t) eCell0][(size_t) srcs[0]]));
                // (a) trajectory on the destination(s) under test
                auto v = uxCheck (tr, 0, srcs, eCell0, c.edits, rampCells);
                double worst = v.worst; int viol = v.violations;
                if (c.d1 > 0)
                {
                    const auto v1 = uxCheck (tr, 1, srcs, eCell0, c.edits, rampCells);
                    worst = juce::jmax (worst, v1.worst); viol += v1.violations;
                }
                cross += tr.crossTalk;
                // (b) audio: max curvature over [first write − 30 ms, last write + 50 ms] vs the held pre / post refs
                const Out* pre = &r50, *post = &rEmpty;
                switch (q)
                {
                    case kDrop0: case kDrop1: case kDrop7: pre = &rEmpty; post = &r50; break;
                    case kDestSwap: post = &r50next; break;
                    case kSrcSwap:  post = &r50s2; break;
                    case kDrag:     pre = &r60; post = &rm60; break;
                    default: break;
                }
                const double t0 = (double) at / 48000.0 - 0.030, t1 = (double) (at + (juce::int64) c.lastCell * ogran::kCell) / 48000.0 + 0.050;
                auto mc = [t0, t1] (const Out& x)
                {
                    const size_t a = (size_t) (t0 * 48000.0), b = (size_t) (t1 * 48000.0);
                    return juce::jmax (eg::maxCurv (x.L, a, b), eg::maxCurv (x.R, a, b));
                };
                const double ratio = mc (o[i]) / juce::jmax (1e-30, juce::jmax (mc (*pre), mc (*post)));
                const bool lin = eg26b::linearWet (o[i]) && eg26b::linearWet (*pre) && eg26b::linearWet (*post);
                wetOk = wetOk && lin;
                worstB[q] = juce::jmax (worstB[q], worst);
                worstK[q] = juce::jmax (worstK[q], ratio);
                tq[q].check (viol == 0 && tr.crossTalk == 0 && ratio <= 2.0 && lin,
                             juce::String::formatted ("%s/%s: %d violations (%.2fx B), cross %d, curv %.2fx%s", m.onset ? "Onset" : "LFO1",
                                                      ds[m.di].name, viol, worst, tr.crossTalk, ratio, lin ? "" : " (wet > 0.5)"));
            }
        }
        const char* tag[] { "UX1a", "UX1a", "UX1a", "UX1b", "UX1b", "UX1b", "UX1c", "UX1d", "UX1e" };
        for (int q = 0; q < kNumSeq; ++q)
            row (tq[q].bad == 0, juce::String (tag[q]) + " " + seqName (q) + ": |dv01| <= 1.05x B, curvature <= 2.0x held pre / post",
                 tq[q].detail (8) + juce::String::formatted ("  [worst %.3fx of B, curvature %.3fx]", worstB[q], worstK[q]));
        std::printf ("  [INFO] UX1 rampCells %d, cross-talk total %d, |source| at the edit: LFO1 %.3f, Onset %.3f%s\n", rampCells, cross,
                     srcAtEdit[0], srcAtEdit[1], wetOk ? "" : "; a render exceeded the linear-wet precondition");

        // NCs (LFO1 at its peak): noResidual on clear, noDepthRamp on drop; > 2x B on >= 1 destination.
        auto nc = [&] (int q, Hooks hk, juce::String& det)
        {
            std::vector<Job> nj;
            std::vector<std::shared_ptr<UxTrace>> nt;
            std::vector<UxCase> nca;
            for (size_t di = 0; di < ds.size(); ++di)
            {
                auto c = uxCase (q, S::Lfo1, 2, ds[di].d, ds[(di + 1) % ds.size()].d);
                auto tr = std::make_shared<UxTrace>();
                nj.push_back (uxJob (false, c.routes, uxEvents (kAtLfo, c.writes), ds[di].d, 0, tr, hk));
                nt.push_back (tr);
                nca.push_back (c);
            }
            eg25::renderAll (nj);
            bool any = false;
            for (size_t di = 0; di < ds.size(); ++di)
            {
                const auto v = uxCheck (*nt[di], 0, { 0, 1 }, (int) (kAtLfo / ogran::kCell), nca[di].edits, rampCells);
                any = any || v.worst > 2.0;
                det << juce::String::formatted ("%s %.1fx ", ds[di].name, v.worst);
            }
            return any;
        };
        juce::String dR, dD;
        const bool nR = nc (kClear0, [] (auto& h) { h.noResidual = true; }, dR);
        row (nR, "UX1nR noResidual on clear: > 2x the bound on >= 1 destination", dR.trim());
        const bool nD = nc (kDrop0, [] (auto& h) { h.noDepthRamp = true; }, dD);
        row (nD, "UX1nD noDepthRamp on drop: > 2x the bound on >= 1 destination", dD.trim());
    }

    //==========================================================================
    // Shared (P133): every TL / MT / ON / LP / LG row injects nowMs (never the
    // 1-argument wall-clock form) and is written for DESTRUCTIVE reads.
    //==========================================================================
    using Tel = GranulatorProcessor::Telemetry;
    using StimFn = std::function<float (juce::int64, int)>;
    constexpr double kT0 = 1.0e6;   // injected reader clock origin (ms)

    inline std::unique_ptr<Proc> mk (const P& ps, int prep = 512, int outCh = 2)
    {
        auto p = make();
        if (outCh == 1)
        {
            juce::AudioProcessor::BusesLayout l;
            l.inputBuses.add (juce::AudioChannelSet::stereo());
            l.outputBuses.add (juce::AudioChannelSet::mono());
            p->setBusesLayout (l);
        }
        setAll (*p, ps);
        p->prepareToPlay (48000.0, prep);
        return p;
    }
    /** Process [pos, pos + len) in `block`-sample blocks (in-place buffer, as a host). */
    inline void run (Proc& p, juce::int64& pos, juce::int64 len, int block, const StimFn& stim,
                     const std::function<void (const juce::AudioBuffer<float>&)>& post = {})
    {
        juce::AudioBuffer<float> buf (2, block);
        for (const juce::int64 end = pos + len; pos < end;)
        {
            const int n = (int) juce::jmin ((juce::int64) block, end - pos);
            eg32::block (p, buf, pos, n, stim);
            if (post) post (buf);
            pos += n;
        }
    }
    inline Tel rd (const Proc& p, double now) { Tel t; p.readTelemetry (t, now); return t; }
    inline StimFn sineAmp (double amp, double hz = 1000.0)
    {
        return [amp, hz] (juce::int64 i, int) { return (float) (amp * std::sin (2.0 * juce::MathConstants<double>::pi * hz * (double) i / 48000.0)); };
    }
    /** 10 ms 1 kHz bursts (amp 0.5) every `periodS`, silence between. */
    inline StimFn bursts (double periodS, double offsetS = 0.05)
    {
        return [periodS, offsetS] (juce::int64 i, int)
        {
            const double t = (double) i / 48000.0 - offsetS;
            if (t < 0.0) return 0.0f;
            const double u = t - periodS * std::floor (t / periodS);
            return u < 0.010 ? (float) (0.5 * std::sin (2.0 * juce::MathConstants<double>::pi * 1000.0 * u)) : 0.0f;
        };
    }
    const float kM6 = (float) std::pow (10.0, -6.0 / 20.0), kM12 = (float) std::pow (10.0, -12.0 / 20.0);
    const double kKnobOnlyDb = 20.0 * std::log10 ((double) ogran::kFbKnob);   // -0.4455 dB

    //==========================================================================
    // MT1–MT5 — meters (D38, R128–R130)
    //==========================================================================
    void mtRows()
    {
        std::printf ("\n-- MT. meters: out = final output, inPeak = post input_gain pre-mix, max since read, stale gap (P133)\n");
        const P dry { { "mix", 0.0f }, { "feedback", 0.0f } };
        // MT1
        {
            auto p = mk (dry);
            juce::int64 pos = 0; double now = kT0;
            rd (*p, now);
            run (*p, pos, 24000, 512, sineAmp (kM6));
            const auto a = rd (*p, now += 33.0);
            setP (*p, "output_gain", 6.0f);
            run (*p, pos, 24000, 512, sineAmp (kM6));
            const auto b = rd (*p, now += 33.0);
            auto m = mk (dry, 512, 1);
            juce::int64 pm = 0; double nm = kT0;
            rd (*m, nm);
            run (*m, pm, 24000, 512, sineAmp (kM6));
            const auto c = rd (*m, nm += 33.0);
            const double g6 = std::pow (10.0, 6.0 / 20.0);
            const bool ok = a.hasMeters && std::abs (a.outPeakL - kM6) <= 1e-5 && std::abs (a.outPeakR - kM6) <= 1e-5
                            && std::abs (b.outPeakL / a.outPeakL - g6) <= 1e-4 && std::abs (b.outPeakR / a.outPeakR - g6) <= 1e-4
                            && juce::exactlyEqual (c.outPeakL, c.outPeakR) && std::abs (c.outPeakL - kM6) <= 1e-5;
            row (ok, "MT1 out meter: -6 dBFS dry sine -> 0.5012 L / R; +6 dB output_gain -> x1.995; mono out R == L",
                 juce::String::formatted ("%.6f / %.6f; x%.5f / x%.5f; mono %.6f / %.6f", a.outPeakL, a.outPeakR,
                                          b.outPeakL / a.outPeakL, b.outPeakR / a.outPeakR, c.outPeakL, c.outPeakR));
        }
        // MT2 / MT2n
        {
            double inP[2] {}, ringP[2] {};
            for (int k = 0; k < 2; ++k)
            {
                auto p = mk (P { { "input_gain", 6.0f }, { "feedback", k == 0 ? 0.0f : 80.0f } });
                juce::int64 pos = 0; double now = kT0;
                run (*p, pos, 48000, 512, sineAmp (kM12));
                rd (*p, now);   // stale: consumes the warm-up
                run (*p, pos, 48000, 512, sineAmp (kM12));
                inP[k] = rd (*p, now += 33.0).inPeak;
                const auto& pr = p->getEngineForTest().getPeakRing();
                for (std::int64_t e = 0; e < ogran::PeakRing::kCells; ++e) ringP[k] = juce::jmax (ringP[k], (double) pr.read (e));
            }
            const double want = (double) kM12 * std::pow (10.0, 6.0 / 20.0);
            row (std::abs (inP[0] - want) <= 1e-5 && juce::exactlyEqual (inP[0], inP[1]),
                 "MT2 inPeak: -12 dBFS + input_gain +6 -> 0.5012; feedback 80 % does not raise it",
                 juce::String::formatted ("fb 0 %.6f, fb 80 %.6f (want %.6f)", inP[0], inP[1], want));
            row (ringP[1] > ringP[0] * 1.05, "MT2n the ring-peak measure (3.2 PeakRing) rises with feedback 80 % (why it is not the in meter)",
                 juce::String::formatted ("fb 0 %.4f, fb 80 %.4f", ringP[0], ringP[1]));
        }
        // MT3 — max since read
        {
            auto p = mk (dry + P { { "s1_enable", 0.0f } });
            juce::int64 pos = 0; double now = kT0;
            rd (*p, now);
            constexpr juce::int64 spike = 3 * 512 + 100;
            run (*p, pos, 10 * 512, 512, [] (juce::int64 i, int) { return i == spike ? 0.9f : 0.0f; });
            const auto a = rd (*p, now += 30.0);
            const auto b = rd (*p, now += 1.0);
            const bool ok = std::abs (a.outPeakL - 0.9f) <= 1e-6 && std::abs (a.outPeakR - 0.9f) <= 1e-6 && std::abs (a.inPeak - 0.9f) <= 1e-6
                            && juce::exactlyEqual (b.outPeakL, 0.0f) && juce::exactlyEqual (b.outPeakR, 0.0f) && juce::exactlyEqual (b.inPeak, 0.0f);
            row (ok, "MT3 max since read: a 1-sample 0.9 spike in block 3 of 10 reads 0.9; an immediate second read reads 0",
                 juce::String::formatted ("out %.6f / %.6f in %.6f; then %.6f / %.6f / %.6f", a.outPeakL, a.outPeakR, a.inPeak, b.outPeakL, b.outPeakR, b.inPeak));
        }
        // MT4 / MT4n — in place
        {
            auto p = mk (P { { "mix", 100.0f }, { "feedback", 0.0f }, { "s1_density", 40.0f }, { "s1_size", 200.0f } });
            juce::int64 pos = 0; double now = kT0;
            run (*p, pos, 48000, 512, noiseStim (0.5f));
            setP (*p, "freeze", 1.0f);
            run (*p, pos, 14400, 512, noiseStim (0.5f));
            rd (*p, now);
            double after = 0.0;
            run (*p, pos, 48000, 512, [] (juce::int64, int) { return 0.0f; },
                 [&after] (const juce::AudioBuffer<float>& b) { after = juce::jmax (after, (double) b.getMagnitude (0, b.getNumSamples())); });
            const auto t = rd (*p, now += 33.0);
            const bool frozen = p->getEngineForTest().isFrozenTel();
            row (frozen && juce::exactlyEqual (t.inPeak, 0.0f) && t.outPeakL > 0.01f,
                 "MT4 in place: silent input under a loud frozen wet tail -> inPeak 0 (out > 0.01)",
                 juce::String::formatted ("inPeak %.6g, out %.4f / %.4f, frozen %d", t.inPeak, t.outPeakL, t.outPeakR, (int) frozen));
            row (after > 0.01, "MT4n measuring the in-place buffer after engine.process reads the output (!= 0)", juce::String::formatted ("%.4f", after));
        }
        // MT5 — staleness (first read included)
        {
            auto p = mk (P { { "mix", 50.0f } });
            auto& e = p->getEngineForTest();
            juce::int64 pos = 0; double now = kT0;
            const auto stim = bursts (0.1);
            run (*p, pos, 24000, 512, stim);
            const auto c0 = e.onsetCountNow();
            const auto a = rd (*p, now);                     // first read after prepare
            run (*p, pos, 4800, 512, stim);
            const auto b = rd (*p, now += 33.0);             // normal
            run (*p, pos, 24000, 512, stim);
            const auto c1 = e.onsetCountNow();
            const auto c = rd (*p, now += 300.0);            // gap > 250 ms
            run (*p, pos, 4800, 512, stim);
            const auto d = rd (*p, now += 33.0);             // normal again
            auto zero = [] (const Tel& t) { return juce::exactlyEqual (t.outPeakL, 0.0f) && juce::exactlyEqual (t.outPeakR, 0.0f) && juce::exactlyEqual (t.inPeak, 0.0f)
                                                   && ! t.onsetFired && juce::exactlyEqual (t.onsetPulse, 0.0f) && t.hasMeters && t.hasOnset; };
            const bool ok = zero (a) && zero (c) && b.outPeakL > 0.0f && b.inPeak > 0.0f && d.outPeakL > 0.0f && d.inPeak > 0.0f
                            && c0 > 0 && c1 != c0;
            row (ok, "MT5 a read > 250 ms after the previous one (and the first read) reports 0 / no onset; the next read is normal",
                 juce::String::formatted ("first %.3f/%d, normal %.3f, gap %.3f/%d (onsets %u -> %u), next %.3f", a.outPeakL, (int) a.onsetFired, b.outPeakL,
                                          c.outPeakL, (int) c.onsetFired, (unsigned) c0, (unsigned) c1, d.outPeakL));
        }
    }

    //==========================================================================
    // ON1 / ON2 — onset (D40 as amended: counter + reader wall clock, 200 ms)
    //==========================================================================
    void onRows()
    {
        std::printf ("\n-- ON. onset: fired once per event-bearing read; 200 ms linear pulse on the reader clock (P133)\n");
        // ON1: 4 Hz bursts, a read every 33 ms of audio (1584 samples)
        {
            auto p = mk (P {});
            auto& e = p->getEngineForTest();
            juce::int64 pos = 0; double now = kT0;
            const auto stim = bursts (0.25);
            rd (*p, now);
            std::uint32_t prevC = e.onsetCountNow();
            double lastFire = -1.0e9;
            int reads = 0, eventReads = 0, fired = 0, mism = 0, pulseBad = 0;
            double worstPulse = 0.0;
            for (int k = 0; k < 90; ++k)
            {
                run (*p, pos, 1584, 1584, stim);
                const std::uint32_t c = e.onsetCountNow();
                const auto t = rd (*p, now += 33.0);
                const bool ev = c != prevC;
                prevC = c;
                ++reads; eventReads += ev ? 1 : 0; fired += t.onsetFired ? 1 : 0;
                if (ev != t.onsetFired) ++mism;
                if (t.onsetFired) lastFire = now;
                const double want = juce::jlimit (0.0, 1.0, 1.0 - (now - lastFire) / 200.0);
                worstPulse = juce::jmax (worstPulse, std::abs ((double) t.onsetPulse - want));
                if (std::abs ((double) t.onsetPulse - want) > 1e-6) ++pulseBad;
            }
            row (mism == 0 && pulseBad == 0 && fired >= 10 && fired == eventReads,
                 "ON1 4 Hz bursts, reads every 33 ms: onsetFired == (counter moved since the last read); pulse == 1 - dt / 200 ms",
                 juce::String::formatted ("%d reads, %d event-bearing, %d fired, %d mismatches; pulse worst |err| %.2g (%d bad); bursts in 2.97 s: 12",
                                          reads, eventReads, fired, mism, worstPulse, pulseBad));
        }
        // ON2 / ON2n: idle host after a fire
        {
            auto p = mk (P {});
            auto& e = p->getEngineForTest();
            juce::int64 pos = 0; double now = kT0;
            rd (*p, now);
            Tel t;
            for (int k = 0; k < 40 && ! t.onsetFired; ++k)
            {
                run (*p, pos, 1584, 1584, bursts (0.25));
                t = rd (*p, now += 33.0);
            }
            const double T = now;
            const juce::int64 absAtFire = pos;
            const float p0 = t.onsetPulse;
            const float p50 = rd (*p, T + 50.0).onsetPulse, p100 = rd (*p, T + 100.0).onsetPulse, p150 = rd (*p, T + 150.0).onsetPulse;
            const float p200 = rd (*p, T + 200.0).onsetPulse, p231 = rd (*p, T + 231.0).onsetPulse;
            (void) e;
            row (t.onsetFired && juce::exactlyEqual (p0, 1.0f) && std::abs (p100 - 0.5f) <= 1e-6 && juce::exactlyEqual (p200, 0.0f) && juce::exactlyEqual (p231, 0.0f)
                 && p50 > p100 && p100 > p150,
                 "ON2 idle host (no process calls): pulse 1 -> 0.5 at +100 ms -> 0 at +200 ms",
                 juce::String::formatted ("%.3f %.3f %.3f %.3f %.3f %.3f", p0, p50, p100, p150, p200, p231));
            // ON2n: a sample-domain pulse (1 - samples since the event / 0.2 fs) never moves without process calls
            const double sp200 = 1.0 - (double) (pos - absAtFire) / (0.2 * 48000.0);
            row (sp200 > 0.5, "ON2n a sample-clock pulse stays > 0.5 with the host idle (why the reader clock)", juce::String::formatted ("%.3f at +200 ms", sp200));
        }
    }

    //==========================================================================
    // LP1 / LP2 — LFO phase (D39, R132)
    //==========================================================================
    struct Head : juce::AudioPlayHead
    {
        double bpm = 120.0; juce::int64 t = 0;
        juce::Optional<PositionInfo> getPosition() const override
        {
            PositionInfo i;
            const double ppq = (double) t / 48000.0 * bpm / 60.0;
            i.setBpm (bpm); i.setPpqPosition (ppq); i.setPpqPositionOfLastBarStart (4.0 * std::floor (ppq / 4.0));
            i.setIsPlaying (true); i.setTimeSignature (TimeSignature { 4, 4 }); i.setTimeInSamples (t);
            return i;
        }
    };

    void lpRows()
    {
        std::printf ("\n-- LP. LFO phase: offset-free cycle position (P133)\n");
        // LP1: Free 1 Hz; phi 0 vs phi 90
        {
            auto a = mk (P { { "lfo1_rate", 1.0f }, { "lfo1_phase", 0.0f } }, 32);
            auto b = mk (P { { "lfo1_rate", 1.0f }, { "lfo1_phase", 90.0f } }, 32);
            juce::int64 pa = 0, pb = 0;
            Tally t;
            double worstPh = 0.0, worstSin = 0.0, worstAB = 0.0;
            const StimFn sil = [] (juce::int64, int) { return 0.0f; };
            // After N cells the newest tick is cell N - 1 (it runs at absCount = (N - 1) * 32).
            for (int N : { 1, 10, 750, 1000, 1499, 2250 })
            {
                run (*a, pa, (juce::int64) N * 32 - pa, 32, sil);
                run (*b, pb, (juce::int64) N * 32 - pb, 32, sil);
                const auto ta = rd (*a, kT0 + N), tb = rd (*b, kT0 + N);
                const double x = (double) (N - 1) * 32.0 / 48000.0, want = x - std::floor (x);
                const double ePh = std::abs ((double) ta.lfoPhase[0] - want);
                const double eAB = std::abs ((double) tb.lfoPhase[0] - (double) ta.lfoPhase[0]);
                const double eS = std::abs ((double) tb.sources[0] - std::sin (2.0 * juce::MathConstants<double>::pi * ((double) tb.lfoPhase[0] + 0.25)));
                worstPh = juce::jmax (worstPh, juce::jmin (ePh, 1.0 - ePh)); worstAB = juce::jmax (worstAB, eAB); worstSin = juce::jmax (worstSin, eS);
                t.check (juce::jmin (ePh, 1.0 - ePh) <= 1e-6 && eAB <= 1e-6 && eS <= 1e-6,
                         juce::String::formatted ("N %d: phase %.7f want %.7f; phi90 %.7f; lfo %.7f", N, ta.lfoPhase[0], want, tb.lfoPhase[0], tb.sources[0]));
            }
            row (t.bad == 0, "LP1 Free 1 Hz: phase == frac ((N - 1) * 32 / fs) after N cells; phi 90 deg: same phase, lfo == sin (2 pi (phase + 0.25))",
                 t.detail (6) + juce::String::formatted ("  [worst phase %.2g, phi-offset %.2g, sin %.2g]", worstPh, worstAB, worstSin));
        }
        // LP2: Tempo 1/4 @ 120 bpm
        {
            const int div = GranulatorParamIds::lfoRateSyncs().indexOf ("1/4");
            auto p = mk (P { { "sync_mode", 1.0f }, { "lfo1_rate_sync", (float) div } }, 32);
            Head h;
            p->setPlayHead (&h);
            juce::int64 pos = 0;
            juce::AudioBuffer<float> buf (2, 32);
            Tally t;
            double worst = 0.0;
            const StimFn sil = [] (juce::int64, int) { return 0.0f; };
            for (int N : { 1, 10, 600, 1000, 1499, 3001 })
            {
                while (pos < (juce::int64) N * 32) { h.t = pos; eg32::block (*p, buf, pos, 32, sil); pos += 32; }
                const auto tt = rd (*p, kT0 + N);
                const double ppq = (double) ((juce::int64) (N - 1) * 32) / 48000.0 * 2.0;   // the newest tick's cell start
                const double rel = ppq - 4.0 * std::floor (ppq / 4.0), want = rel - std::floor (rel);
                const double err = std::abs ((double) tt.lfoPhase[0] - want);
                worst = juce::jmax (worst, juce::jmin (err, 1.0 - err));
                t.check (juce::jmin (err, 1.0 - err) <= 1e-6, juce::String::formatted ("N %d: %.7f want %.7f", N, tt.lfoPhase[0], want));
            }
            p->setPlayHead (nullptr);
            row (div >= 0 && t.bad == 0, "LP2 Tempo 1/4 @ 120 bpm: phase == frac ((ppq - bar) / 1 beat) at the newest tick",
                 t.detail (6) + juce::String::formatted ("  [worst %.2g]", worst));
        }
    }

    //==========================================================================
    // LG1–LG3 — loop gain (D34 as amended: k_eff x AGC)
    //==========================================================================
    void lgRows()
    {
        std::printf ("\n-- LG. loop gain = 20 log10 (k_eff * g_agc); absent at k 0 / frozen; <= -0.4455 dB (P133)\n");
        // LG1
        double lgQuiet = 0.0, lgHot = 0.0; bool hq = false, hh = false;
        {
            auto p = mk (P { { "feedback", 50.0f } });
            juce::int64 pos = 0; double now = kT0;
            run (*p, pos, 24000, 512, sineAmp (0.001));
            const auto t = rd (*p, now);
            hq = t.hasLoopGain; lgQuiet = t.loopGainDb;
            // Hot: fb_saturation 0 (d = 1). At any d > 1 / 0.7 (fb_saturation > 6.1 %, the default is
            // 20 %) the tanh (d x) / d stage caps the AGC input at 1 / d < 0.7 and the AGC never engages.
            auto q = mk (P { { "feedback", 100.0f }, { "mix", 100.0f }, { "input_gain", 12.0f }, { "fb_saturation", 0.0f },
                             { "fb_diffusion", 0.0f }, { "s1_spray", 0.0f }, { "s1_jitter", 0.0f } });
            juce::int64 pq = 0;
            run (*q, pq, 96000, 512, sineAmp (1.0, 100.0));
            const auto u = rd (*q, now);
            hh = u.hasLoopGain; lgHot = u.loopGainDb;
        }
        const double want50 = 20.0 * std::log10 (0.5 * (double) ogran::kFbKnob);
        row (hq && std::abs (lgQuiet - want50) <= 0.01 && hh && lgHot < kKnobOnlyDb,
             "LG1 fb 50 % quiet input -> -6.465 dB; fb 100 % hot input, saturation 0 -> < 20 log10 (0.95) (the AGC engaged)",
             juce::String::formatted ("%.4f dB (want %.4f); hot %.4f dB (knob-only %.4f)", lgQuiet, want50, lgHot, kKnobOnlyDb));
        // LG2
        {
            auto z = mk (P { { "feedback", 0.0f } });
            juce::int64 pz = 0;
            run (*z, pz, 4800, 512, sineAmp (0.1));
            const auto t0 = rd (*z, kT0);
            auto p = mk (P { { "feedback", 50.0f } }, 32);
            auto& e = p->getEngineForTest();
            juce::int64 pos = 0; double now = kT0;
            run (*p, pos, 9600, 32, sineAmp (0.1));
            setP (*p, "freeze", 1.0f);
            run (*p, pos, 64, 32, sineAmp (0.1));             // 2 cells: pending (defer 9 cells), head not stopped
            const bool pend = ! e.isFrozenTel();
            const auto a = rd (*p, now += 1.0);
            run (*p, pos, 9600, 32, sineAmp (0.1));
            const auto b = rd (*p, now += 33.0);
            const bool fr = e.isFrozenTel();
            setP (*p, "freeze", 0.0f);
            run (*p, pos, 4800, 32, sineAmp (0.1));
            const auto c = rd (*p, now += 33.0);
            const bool ok = ! t0.hasLoopGain && std::isnan (t0.loopGainDb) && pend && a.hasLoopGain && std::isfinite (a.loopGainDb)
                            && fr && ! b.hasLoopGain && std::isnan (b.loopGainDb) && c.hasLoopGain && std::isfinite (c.loopGainDb);
            row (ok, "LG2 fb 0 -> absent; freezePending -> present; frozen -> absent; unfreeze -> present",
                 juce::String::formatted ("fb0 %d; pending %d/%d %.3f; frozen %d/%d; unfrozen %d %.3f", (int) t0.hasLoopGain, (int) pend, (int) a.hasLoopGain,
                                          a.loopGainDb, (int) fr, (int) b.hasLoopGain, (int) c.hasLoopGain, c.loopGainDb));
        }
        // LG3 / LG3n
        {
            auto p = mk (P { { "feedback", 0.0f }, { "lfo1_rate", 2.0f } } + route (1, S::Lfo1, D::Feedback, 50.0f));
            juce::int64 pos = 0; double now = kT0;
            rd (*p, now);
            int present = 0, bad = 0, reads = 0;
            double hi = -1e9;
            for (int k = 0; k < 60; ++k)
            {
                run (*p, pos, 1584, 512, sineAmp (0.01));
                const auto t = rd (*p, now += 33.0);
                ++reads;
                if (! t.hasLoopGain) continue;
                ++present;
                hi = juce::jmax (hi, (double) t.loopGainDb);
                if (! std::isfinite (t.loopGainDb) || t.loopGainDb > kKnobOnlyDb + 1e-6) ++bad;
            }
            row (present > 0 && bad == 0, "LG3 knob 0 + LFO1 -> Feedback 50 %: present, finite, <= -0.4455 dB (the modulated k)",
                 juce::String::formatted ("%d / %d reads present, %d bad, max %.3f dB", present, reads, bad, hi));
            // LG3n: the unmodulated knob ramp (0.95 * feedback param = 0) would read absent every time
            auto* fbp = param (*p, "feedback");
            const double kRamp = (double) ogran::kFbKnob * (double) fbp->convertFrom0to1 (fbp->getValue()) / 100.0;
            row (juce::exactlyEqual (kRamp, 0.0), "LG3n a ramp-based k (knob only) reads absent under the same route", juce::String::formatted ("k_ramp %.3f -> 0 / %d present", kRamp, reads));
        }
    }

    //==========================================================================
    // TL33 (replaces TL32, R135)
    //==========================================================================
    void tlRows33()
    {
        std::printf ("\n-- TL. telemetry (Stage 3 / 3.3, replaces TL32)\n");
        {
            auto un = make();
            Tel t;
            const bool ok = un->readTelemetry (t, kT0);
            row (! ok && ! t.hasBuffer && ! t.hasCounts && ! t.hasMeters && ! t.hasOnset && ! t.hasLoopGain,
                 "TL33u readTelemetry false only before prepare; every has* false then");
        }
        auto p = make();
        p->prepareToPlay (48000.0, 256);
        setParam (*p, "mod3_source", 1.0f);  setParam (*p, "mod3_dest", 5.0f);
        setParam (*p, "mod7_source", 9.0f);  setParam (*p, "mod7_dest", 40.0f);
        setParam (*p, "lfo1_rate", 1.0f);
        setParam (*p, "macro2", 63.0f);
        setParam (*p, "feedback", 30.0f);
        juce::AudioBuffer<float> buf (2, 256);
        juce::MidiBuffer midi;
        juce::Random rng (31);
        Tel t0;
        p->readTelemetry (t0, kT0);   // the first read (stale) — consumes nothing yet
        for (int b = 0; b < 200; ++b)
        {
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 256; ++i)
                    buf.setSample (ch, i, 0.3f * std::sin (0.0577f * (float) (b * 256 + i)) + 0.01f * (rng.nextFloat() - 0.5f));
            p->processBlock (buf, midi);
        }
        Tel t;
        t.counts[0] = 99; t.loopGainDb = 1.0f; t.hasMeters = false; t.onsetFired = true;   // dirty: readTelemetry must reset
        const bool ok = p->readTelemetry (t, kT0 + 33.0);
        const auto& e = p->getEngineForTest();
        Tally tl;
        for (int i = 0; i < 11; ++i)
            tl.check (juce::exactlyEqual (t.sources[(size_t) i], e.getSrcTelemetry (i + 1)), "src " + juce::String (i + 1));
        const float note = e.getPitchNote();
        tl.check (juce::exactlyEqual (t.pitchMidi, note > 0.0f ? note : 0.0f), "pitchMidi");
        tl.check (note <= 0.0f || juce::exactlyEqual (t.pitchClarity, e.getPitchConfident() ? 1.0f : 0.0f), "pitchClarity");
        tl.check (t.scaleRev == p->getScaleStateRevision(), "scaleRev");
        int routed = 0;
        for (int d = 1; d < Tel::kDests; ++d)
            if (t.modRouted[(size_t) d]) { ++routed; tl.check (juce::exactlyEqual (t.modNorm[(size_t) d], e.getDestTelemetry (d)), "modNorm " + juce::String (d)); }
        tl.check (routed == 2 && t.modRouted[5] && t.modRouted[40], "routed set " + juce::String (routed));
        int lit = 0, cnt = 0;
        for (auto v : t.wave) lit += v > 0.0f ? 1 : 0;
        for (auto c : t.counts) cnt += c;
        tl.check (t.hasBuffer && t.waveCount == eg32::kBins && lit > 0, "wave live");
        tl.check (t.grainCount > 0 && t.grainCount <= Tel::kMaxGrains, "grains live " + juce::String (t.grainCount));
        tl.check (juce::exactlyEqual (t.spanSec, 8.0f) && ! t.frozen, "span 8 s / not frozen " + juce::String (t.spanSec));
        tl.check (juce::exactlyEqual (t.bands[0][0], 0.0f) && juce::exactlyEqual (t.bands[0][1], 0.1f), "bands s1 base");
        tl.check (t.hasCounts && t.limit == 128 && cnt > 0 && cnt <= 128, "counts / limit " + juce::String (cnt) + " / " + juce::String (t.limit));
        // 3.3 live
        tl.check (t.hasMeters && t.outPeakL > 0.0f && t.outPeakR > 0.0f && t.inPeak > 0.29f && t.inPeak < 0.32f, juce::String::formatted ("meters %.3f %.3f %.3f", t.outPeakL, t.outPeakR, t.inPeak));
        tl.check (t.hasOnset && t.onsetPulse >= 0.0f && t.onsetPulse <= 1.0f, "onset flags");
        tl.check (t.lfoPhase[0] > 0.0f && t.lfoPhase[0] < 1.0f && juce::exactlyEqual (t.lfoPhase[0], e.lfoPhase (0)), juce::String::formatted ("lfoPhase %.4f", t.lfoPhase[0]));
        const double wantLg = 20.0 * std::log10 ((double) e.loopK() * (double) e.loopAgc());
        tl.check (t.hasLoopGain && std::abs (t.loopGainDb - wantLg) <= 1e-4 && t.loopGainDb <= kKnobOnlyDb, juce::String::formatted ("loopGain %.4f", t.loopGainDb));
        bool live = false;
        for (auto v : t.sources) live = live || v != 0.0f;
        tl.check (live, "liveness: every source reads 0 after 200 blocks");
        row (ok && tl.bad == 0, "TL33 readTelemetry: 3.1 fields == engine; 3.2 live; 3.3 meters / onset / lfoPhase / loop gain live",
             tl.detail (11 + 5 + routed + 5 + 5) + "  grains " + juce::String (t.grainCount) + juce::String::formatted (", out %.3f in %.3f, phase %.3f, loop %.2f dB",
                                                                                                                    t.outPeakL, t.inPeak, t.lfoPhase[0], t.loopGainDb));
    }

    //==========================================================================
    // M9 — allocations with every 3.3 write active (M8 extended)
    //==========================================================================
    void m9Rows()
    {
        std::printf ("\n-- M9. allocations with the 3.3 telemetry active (CAS meters, onset counter, LFO phase, loop gain, out scan), armed from block 0\n");
       #if JUCE_MAC
        audioThread = pthread_self();
        malloc_logger = countAllocation;
        allocCount = 0;
        allocTrace = false;
        allocArmed = true;
        void* volatile probe = std::malloc (64);
        allocArmed = false;
        std::free (probe);
        const int liveness = allocCount;
        allocCount = 0;
        allocTrace = true;
        int total = 0;
        juce::String det;
        bool live = true;
        for (int sched = 0; sched < 2; ++sched)
        {
            Job j;
            j.fs = 48000.0; j.len = 6 * 48000; j.seed = 0xA779ull;
            j.block = sched == 0 ? 0 : 8192;
            j.prepBlock = 512;
            j.params = eg26::c7Params() + P { { "feedback", 50.0f } };
            j.stim = eg26::c7Stim (j.fs);
            j.events.push_back ({ 2 * 48000 + 13, [] (Proc& p) { setP (p, "freeze", 1.0f); } });
            j.events.push_back ({ 3 * 48000 + 7, [] (Proc& p) { setP (p, "freeze", 0.0f); } });
            struct Seen { int reads = 0, out = 0, in = 0, fired = 0, phase = 0, loop = 0; double now = kT0; };
            auto seen = std::make_shared<Seen>();
            j.after = [seen] (Proc& p, juce::int64)   // message side, NOT armed
            {
                Tel tel;
                p.readTelemetry (tel, seen->now += 10.0);
                ++seen->reads;
                seen->out += tel.outPeakL > 0.0f ? 1 : 0;
                seen->in += tel.inPeak > 0.0f ? 1 : 0;
                seen->fired += tel.onsetFired ? 1 : 0;
                seen->phase += tel.lfoPhase[0] > 0.0f ? 1 : 0;
                seen->loop += tel.hasLoopGain ? 1 : 0;
            };
            armDuringProcess = true;
            const auto o = render (j);
            armDuringProcess = false;
            total += allocCount;
            live = live && seen->out > 0 && seen->in > 0 && seen->fired > 0 && seen->phase > 0 && seen->loop > 0;
            det << (sched == 0 ? "var" : "8192 (prep 512)") << juce::String::formatted (": %d allocs, %d reads (out %d, in %d, fired %d, phase %d, loop %d); ",
                                                                               (int) allocCount, seen->reads, seen->out, seen->in, seen->fired, seen->phase, seen->loop);
            allocCount = 0;
        }
        malloc_logger = nullptr;
        allocTrace = false;
        row (liveness == 1, "M9L liveness: a deliberate malloc on the render thread counts exactly 1", juce::String (liveness));
        row (liveness == 1 && total == 0 && live, "M9 0 allocations from block 0 with every 3.3 write active (var, fresh 8192 after 512, freeze edges)", det.trim());
       #else
        row (false, "M9 alloc gate is macOS-only");
       #endif
    }

    //==========================================================================
    // C8 — INFO: CPU at the C7 config (3.3 delta)
    //==========================================================================
    void c8Info()
    {
        if (gNoC7) { std::printf ("  [SKIP] C8 (--no-c7 iteration aid)\n"); return; }
        const auto r = eg26b::timeC7 (eg26b::c7Params(), 48000.0, 128, 3, 10.0, 5.0, 2);
        if (r.pct.empty()) { std::printf ("  [INFO] C8 contended (0 valid runs)\n"); return; }
        std::printf ("  [INFO] C8 CPU at the C7 config, N 3 x 10 s: median %.2f %% (C7 at baseline C 17.19 %%, 3.2 T4 17.02 %%): delta vs C %+.2f\n",
                     r.median(), r.median() - 17.19);
    }
} } // namespace eg33 (anonymous)

static void engineRows33()
{
    using namespace eg33;
    ux1Rows();
    mtRows();
    onRows();
    lpRows();
    lgRows();
    m9Rows();
    tlRows33();
    std::printf ("\n-- C8. INFO\n");
    c8Info();
}
