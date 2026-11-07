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
// Stage 4 rows (PLAN-4 P145, P148, P155–P157).
//
//   AG1    D43: FB4 LP-res-100 corner (sat 20 %, fb 100 %, 20 s): min g_agc
//          over 10–20 s < 0.9; return peak <= kAgcCeil/d · (1 + m); finite
//   AG1n   the same render on the legacy staging: g_agc == 1 throughout
//          (fails AG1's predicate)
//   AG2    FB3n phase-locked ARCH growth at sat 0 / 20 / 50 / 100 %: engaged
//          at every sat, return holds at 0.7/d ± tol, no growth after the hold
//          (per-generation energy slope <= +0.05 dB/gen)
//   AG3    sat 0: carried FB-family renders bitwise == the legacy staging
//   AG4    pumping at defaults + fb 50 %: (a) −6 dBFS-peak noise bursts →
//          g_agc == 1; (b) 0 dBFS-peak kick/snare loop → depth <= 1 dB p-p
//          over 400 ms windows
//   AG5    Z7d config at default sat: exact zero, no later than the legacy
//          staging; a freeze render at default sat bitwise == legacy
//   AG6    env <= 1 + 1e-6 across the FB4 corner set (structural bound)
//==============================================================================
namespace { namespace eg4
{
    using namespace eg;
    using eg25b::operator+;
    using eg22::sp;
    constexpr double kFs = 48000.0;
    inline void info (const juce::String& s) { std::printf ("  [INFO] %s\n", s.toRawUTF8()); }

    inline bool sameBits (const Out& a, const Out& b)
    {
        return a.L.size() == b.L.size()
            && std::memcmp (a.L.data(), b.L.data(), a.L.size() * sizeof (float)) == 0
            && std::memcmp (a.R.data(), b.R.data(), a.R.size() * sizeof (float)) == 0;
    }

    //==========================================================================
    // AG — D43 AGC restage (P144, P145)
    //==========================================================================
    /** Per-block AGC trace. With block == kCell (32) every block is one cell, so
        fbDelay holds exactly that block's chain return after processBlock. */
    struct AgcTrace
    {
        std::vector<float> g, env, ret;   // per block: g_agc, env, max |return|
        bool finite = true;
        float gMin (size_t from = 0) const { float m = 1.0f; for (size_t i = from; i < g.size(); ++i) m = std::min (m, g[i]); return m; }
        float envMax() const { float m = 0.0f; for (auto v : env) m = std::max (m, v); return m; }
        float retMax (size_t from, size_t to) const { float m = 0.0f; for (size_t i = from; i < to && i < ret.size(); ++i) m = std::max (m, ret[i]); return m; }
    };

    inline void attachAgc (Job& j, std::shared_ptr<AgcTrace> t)
    {
        auto prev = j.after;
        j.after = [prev, t] (Proc& p, juce::int64 pos)
        {
            if (prev) prev (p, pos);
            const auto& e = p.getEngineForTest();
            t->g.push_back (e.loopAgc());
            t->env.push_back (e.getChainForTest().env);
            const auto& fd = e.getFbDelayForTest();
            float m = 0.0f;
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < ogran::kFbDelay; ++i)
                {
                    if (! std::isfinite (fd[c][i])) t->finite = false;
                    m = std::max (m, std::abs (fd[c][i]));
                }
            t->ret.push_back (m);
        };
    }
    inline void legacyAgc (Job& j)
    {
        auto prev = j.pre;
        j.pre = [prev] (Proc& p) { if (prev) prev (p); p.getEngineForTest().hooks.agcLegacy = true; };
    }

    /** The FB4 corner set (engine_gates_24.h FB4): fb 100 %, default sat, 20 s,
        −52 dBFS RMS noise for 10 s then −6 dBFS RMS. Block 32 for the trace. */
    inline std::vector<std::pair<juce::String, P>> fb4Corners()
    {
        return { { "drive100", { { "s1_drive", 100.0f } } },
                 { "LPres100", { { "s1_filter_type", 1.0f }, { "s1_filter_res", 100.0f }, { "s1_filter_cutoff", 1000.0f } } },
                 { "HPres100", { { "s1_filter_type", 3.0f }, { "s1_filter_res", 100.0f }, { "s1_filter_cutoff", 1000.0f } } },
                 { "crush100", { { "s1_crush", 100.0f } } } };
    }
    inline Job fb4Job (const P& extra, int block = 512, float sat = 20.0f)
    {
        Job j; j.len = (juce::int64) (20.0 * kFs); j.block = block; j.prepBlock = block;
        j.params = eg24::fbBase (100.0f) + P { { "s1_density", 12.5f }, { "s1_size", 100.0f }, { "output_gain", 0.0f }, { "fb_saturation", sat } } + extra;
        constexpr juce::int64 sw = (juce::int64) (10.0 * kFs);
        j.stim = [] (juce::int64 i, int ch) { return eg::hashNoise (i, ch, i < sw ? eg24::kNoise52 : 0.5f * 1.7320508f); };
        return j;
    }

    /** FB3n: ARCH law, Tukey, phase-locked, N 1.25, 0.3 s of input at −120 dBFS. */
    inline Job fb3nJob (float sat, double lenS)
    {
        const double H = kFs / 12.5, span = 8.0 * kFs;
        const float lockPct = (float) (100.0 * (H - (ogran::kFbDelay - 1) - ogran::Ring::margin (0)) / span);
        Job j; j.len = (juce::int64) (lenS * kFs); j.block = 32; j.prepBlock = 32;
        j.params = eg24::fbBase (100.0f) + P { { "s1_density", 12.5f }, { "s1_size", 100.0f }, { "s1_window", 0.333f },
                                               { "s1_position", lockPct }, { "fb_saturation", sat } };
        j.stim = eg24::gatedNoise (kFs, 0.3, eg24::kNoise52 * 2.5119e-4f);
        j.pre = [] (Proc& p) { p.getEngineForTest().hooks.fbArchLaw = true; };
        return j;
    }

    /** Seeded kick / snare loop at 120 bpm (kick 1 + 3, snare 2 + 4, closed hat on
        eighths), normalised to a 0 dBFS sample peak. Position-deterministic. */
    struct DrumLoop
    {
        std::vector<float> L, R;
        explicit DrumLoop (double fs, double bars = 2.0)
        {
            const double beat = 60.0 / 120.0;
            const size_t n = (size_t) (bars * 4.0 * beat * fs);
            L.assign (n, 0.0f); R.assign (n, 0.0f);
            auto add = [&] (double t0, auto&& gen, double durS)
            {
                const size_t a = (size_t) (t0 * fs), len = (size_t) (durS * fs);
                for (size_t k = 0; k < len && a + k < n; ++k)
                {
                    const double t = (double) k / fs;
                    float l = 0.0f, r = 0.0f;
                    gen (t, (juce::int64) (a + k), l, r);
                    L[a + k] += l; R[a + k] += r;
                }
            };
            for (int b = 0; b < (int) (bars * 4.0); ++b)
            {
                const double t0 = b * beat;
                if (b % 2 == 0)   // kick: 150 → 45 Hz sweep, 300 ms
                    add (t0, [] (double t, juce::int64, float& l, float& r)
                    {
                        const double ph = 2.0 * eg::kPi * (45.0 * t + (105.0 * 0.035) * (1.0 - std::exp (-t / 0.035)));
                        const float v = (float) (std::sin (ph) * std::exp (-t / 0.12));
                        l += v; r += v;
                    }, 0.3);
                else              // snare: 190 Hz body + hashed noise, 200 ms
                    add (t0, [] (double t, juce::int64 i, float& l, float& r)
                    {
                        const double body = 0.5 * std::sin (2.0 * eg::kPi * 190.0 * t) * std::exp (-t / 0.05);
                        const double env = std::exp (-t / 0.06);
                        l += (float) (body + env * eg::hashNoise (i, 0, 0.8f));
                        r += (float) (body + env * eg::hashNoise (i, 1, 0.8f));
                    }, 0.2);
                for (int h = 0; h < 2; ++h)   // hat on eighths: 40 ms of noise, quiet
                    add (t0 + h * 0.5 * beat, [] (double t, juce::int64 i, float& l, float& r)
                    {
                        const double env = 0.25 * std::exp (-t / 0.012);
                        l += (float) (env * eg::hashNoise (i + 7919, 0, 1.0f));
                        r += (float) (env * eg::hashNoise (i + 7919, 1, 1.0f));
                    }, 0.04);
            }
            float pk = 0.0f;
            for (size_t k = 0; k < n; ++k) pk = std::max ({ pk, std::abs (L[k]), std::abs (R[k]) });
            for (size_t k = 0; k < n; ++k) { L[k] /= pk; R[k] /= pk; }
        }
        std::function<float (juce::int64, int)> stim() const
        {
            auto self = std::make_shared<DrumLoop> (*this);
            return [self] (juce::int64 i, int ch) { const auto& v = ch ? self->R : self->L; return v[(size_t) (i % (juce::int64) v.size())]; };
        }
    };

    /** g_agc modulation depth: max over 400 ms windows (after `skip` blocks) of
        20 log10 (max g / min g) inside the window. */
    inline double agcDepthDb (const AgcTrace& t, int blocksPerWin, size_t skip)
    {
        double worst = 0.0;
        for (size_t a = skip; a + (size_t) blocksPerWin <= t.g.size(); a += (size_t) blocksPerWin)
        {
            float lo = 1.0f, hi = 0.0f;
            for (size_t k = a; k < a + (size_t) blocksPerWin; ++k) { lo = std::min (lo, t.g[k]); hi = std::max (hi, t.g[k]); }
            worst = std::max (worst, 20.0 * std::log10 ((double) hi / (double) lo));
        }
        return worst;
    }

    void agRows()
    {
        std::printf ("\n-- AG. D43 AGC restage: follower on d·|y| (ceiling tracks saturation), sat 0 bitwise\n");
        const float d20 = 1.0f + ogran::kSatSpan * 0.2f;

        // AG1 / AG1n / AG6 — the FB4 corner set at block 32 (+ drive100 on the legacy staging).
        {
            auto cs = fb4Corners();
            std::vector<Job> jobs; std::vector<double> cost;
            std::vector<std::shared_ptr<AgcTrace>> tr;
            for (auto& c : cs) { auto j = fb4Job (c.second, 32); tr.push_back (std::make_shared<AgcTrace>()); attachAgc (j, tr.back()); jobs.push_back (j); cost.push_back (1.0); }
            // AG1 corner: LP res 100 (PLAN-4 P145 named drive-100, but the grain drive's own tanh keeps that
            // loop below unity — env 0.42 at sat 20 %, g_agc == 1 under D43 too — so it cannot show the AGC;
            // the LP-res corner is the FB4 corner whose loop runs away). SUMMARY-4 deviation.
            constexpr size_t kAg1 = 1;
            { auto j = fb4Job (cs[kAg1].second, 32); legacyAgc (j); tr.push_back (std::make_shared<AgcTrace>()); attachAgc (j, tr.back()); jobs.push_back (j); cost.push_back (1.0); }
            std::vector<bool> fin (jobs.size(), true);
            eg22b::renderPool (jobs, cost, [&] (size_t i, Out& o)
            {
                for (size_t k = 0; k < o.L.size(); ++k) if (! std::isfinite (o.L[k]) || ! std::isfinite (o.R[k])) fin[i] = false;
            });
            const size_t b10 = (size_t) (10.0 * kFs) / ogran::kFbDelay;
            // m: the attack overshoot measured at T1 (PLAN-4 P145: fixed at m_meas + 0.01).
            constexpr double kAg1M = 0.209;   // T1 measured 0.1986 (1 ms attack overshoot) + 0.01 (PLAN-4 P145)
            auto ag1 = [&] (const AgcTrace& t, bool f, double& gmin, double& retN)
            {
                gmin = t.gMin (b10);
                retN = (double) t.retMax (b10, t.ret.size()) * d20 / ogran::kAgcCeil;   // return / (kAgcCeil / d)
                return gmin < 0.9 && retN <= 1.0 + kAg1M && f && t.finite;
            };
            double gm, rn, gmL, rnL;
            const bool ok1 = ag1 (*tr[kAg1], fin[kAg1], gm, rn);
            const bool okL = ag1 (*tr[4], fin[4], gmL, rnL);
            row (ok1, "AG1 FB4 LP-res-100 corner, sat 20 %, fb 100 %, 20 s: min g_agc (10-20 s) < 0.9, return <= kAgcCeil/d (1 + m), finite",
                 juce::String::formatted ("min g %.4f, return / (0.7/d) %.4f (m %.3f)", gm, rn, kAg1M));
            row (! okL && tr[4]->gMin() == 1.0f, "AG1n LP-res-100 corner, legacy staging: g_agc == 1 for the whole render, fails AG1",
                 juce::String::formatted ("min g %.6f, return / (0.7/d) %.4f", (double) tr[4]->gMin(), rnL));
            bool ok6 = true; juce::String det;
            for (size_t i = 0; i < cs.size(); ++i)
            {
                const float em = tr[i]->envMax();
                ok6 = ok6 && em <= 1.0f + 1.0e-6f && fin[i];
                det << cs[i].first << juce::String::formatted (" env %.6f g %.3f; ", (double) em, (double) tr[i]->gMin());
            }
            row (ok6, "AG6 FB4 corners: env <= 1 + 1e-6 (structural; replaces env <= 0.7 + 1/d)", det.trim());
        }

        // AG2 — FB3n growth at sat 0 / 20 / 50 / 100 %.
        {
            const float sats[4] { 0.0f, 20.0f, 50.0f, 100.0f };
            std::vector<Job> jobs; std::vector<double> cost;
            std::vector<std::shared_ptr<AgcTrace>> tr;
            for (float s : sats) { auto j = fb3nJob (s, 10.0); tr.push_back (std::make_shared<AgcTrace>()); attachAgc (j, tr.back()); jobs.push_back (j); cost.push_back (1.0); }
            std::vector<eg24::Fit> fits (jobs.size());
            const double genS = (kFs / 12.5) / kFs;
            eg22b::renderPool (jobs, cost, [&] (size_t i, Out& o) { fits[i] = eg24::decayFit (o, kFs, 6.0, 10.0, -240.0, genS); });
            constexpr double kAg2Tol = 0.224;   // T1 measured |hold - 1| max 0.2134 (sat 0) + 0.01
            bool ok = true; juce::String det;
            const size_t a = (size_t) (6.0 * kFs) / ogran::kFbDelay, b = (size_t) (10.0 * kFs) / ogran::kFbDelay;
            for (size_t i = 0; i < 4; ++i)
            {
                const float d = 1.0f + ogran::kSatSpan * sats[i] / 100.0f;
                const double hold = (double) tr[i]->retMax (a, b) * d / ogran::kAgcCeil;   // 1.0 == 0.7/d
                const double dbGen = fits[i].slope * genS;
                const bool good = tr[i]->gMin() < 1.0f && std::abs (hold - 1.0) <= kAg2Tol && dbGen <= 0.05 && tr[i]->finite;
                ok = ok && good;
                det << juce::String::formatted ("sat %g: min g %.3f hold %.4f %+.4f dB/gen%s; ", (double) sats[i], (double) tr[i]->gMin(), hold, dbGen, good ? "" : " BAD");
            }
            row (ok, "AG2 FB3n ARCH growth at sat 0/20/50/100 %: engaged, return holds at 0.7/d +-tol, <= +0.05 dB/gen after the hold",
                 det.trim() + juce::String::formatted (" (tol %.3f)", kAg2Tol));
        }

        // AG3 — sat 0: the carried FB-family renders, new == legacy bitwise.
        {
            std::vector<std::pair<juce::String, Job>> base;
            {   // FB1
                Job j; j.len = (juce::int64) (2.0 * kFs);
                j.params = eg24::fbBase (50.0f) + P { { "s1_density", 50.0f }, { "s1_size", 50.0f }, { "s1_position", 1.25f } };
                j.stim = eg24::burstStim (kFs, 1.0, 0.010, 0.1f * 1.7320508f);
                base.push_back ({ "FB1", j });
            }
            for (auto ex : { P { { "fb_low_cut", 80.0f } }, P { { "fb_low_cut", 1000.0f } }, P { { "fb_high_cut", 12000.0f } }, P { { "fb_high_cut", 2000.0f } } })
            {   // FB2a / b tone
                Job j; j.len = (juce::int64) (6.0 * kFs);
                j.params = eg24::fbBase (100.0f) + P { { "s1_density", 50.0f }, { "s1_size", 200.0f }, { "s1_position", 5.0f } } + ex;
                j.stim = eg::noiseStim (0.1f * 1.7320508f);
                base.push_back ({ "FB2ab", j });
            }
            for (auto ex : { P {}, P { { "fb_diffusion", 100.0f } } })
            {   // FB2c / d burst
                Job j; j.len = (juce::int64) (3.0 * kFs);
                j.params = eg24::fbBase (80.0f) + P { { "s1_density", 50.0f }, { "s1_size", 50.0f } } + ex;
                j.stim = eg24::burstStim (kFs, 1.0, 0.020, 0.5f * 1.7320508f);
                base.push_back ({ "FB2cd", j });
            }
            {   // FB3 corners (pos 0 / locked × 4 windows × 4 FX) + 5 extremes
                const float wins[4] { 0.0f, 0.333f, 0.667f, 1.0f };
                const double H = kFs / 12.5, span = 8.0 * kFs;
                const float lockPct = (float) (100.0 * (H - (ogran::kFbDelay - 1) - ogran::Ring::margin (0)) / span);
                std::vector<P> ex;
                for (int lock = 0; lock < 2; ++lock)
                    for (float w : wins)
                    {
                        const float pos = lock ? lockPct : 0.0f;
                        ex.push_back ({ { "s1_window", w }, { "s1_position", pos } });
                        ex.push_back ({ { "s1_window", w }, { "s1_position", pos }, { "s1_filter_type", 2.0f }, { "s1_filter_res", 100.0f }, { "s1_filter_cutoff", 1000.0f } });
                        ex.push_back ({ { "s1_window", w }, { "s1_position", pos }, { "s1_filter_type", 1.0f }, { "s1_filter_res", 30.0f }, { "s1_filter_cutoff", 1000.0f } });
                        ex.push_back ({ { "s1_window", w }, { "s1_position", pos }, { "s1_ring_mix", 100.0f }, { "s1_ring_freq", 300.0f } });
                    }
                ex.push_back ({ { "s1_window", 0.333f }, { "fb_low_cut", 2000.0f } });
                ex.push_back ({ { "s1_window", 0.333f }, { "fb_high_cut", 1000.0f } });
                ex.push_back ({ { "s1_window", 0.333f }, { "fb_diffusion", 100.0f } });
                ex.push_back ({ { "s1_window", 0.333f }, { "s1_pitch", 12.0f } });
                ex.push_back ({ { "s1_window", 0.333f }, { "s1_pitch", 24.0f } });
                for (auto& e : ex)
                {
                    Job j; j.len = (juce::int64) (5.5 * kFs);
                    j.params = eg24::fbBase (100.0f) + P { { "s1_density", 12.5f }, { "s1_size", 100.0f } } + e;
                    j.stim = eg24::gatedNoise (kFs, 3.0, eg24::kNoise52);
                    base.push_back ({ "FB3", j });
                }
                base.push_back ({ "FB3n", fb3nJob (0.0f, 5.5) });
            }
            for (auto& c : fb4Corners()) base.push_back ({ "FB4", fb4Job (c.second, 512, 0.0f) });
            {   // LG1 hot (the AGC engages at sat 0 today)
                Job j; j.len = 96000; j.block = 512;
                j.params = P { { "feedback", 100.0f }, { "mix", 100.0f }, { "input_gain", 12.0f }, { "fb_diffusion", 0.0f }, { "s1_spray", 0.0f }, { "s1_jitter", 0.0f } };
                j.stim = eg::sineStim (100.0, kFs, 1.0);
                base.push_back ({ "LG1hot", j });
            }
            {   // Z7d config (30 s: 10 s input + 20 s decay)
                P cfg { { "mix", 100.0f }, { "grain_limit", 3.0f }, { "buffer_length", 30.0f }, { "feedback", 100.0f }, { "fb_diffusion", 100.0f },
                        { "reverb_mix", 100.0f }, { "reverb_size", 100.0f }, { "reverb_damp", 0.0f } };
                for (int s = 1; s <= 4; ++s)
                    cfg = cfg + P { { sp (s, "enable"), 1.0f }, { sp (s, "density"), 200.0f }, { sp (s, "size"), 2000.0f }, { sp (s, "spray"), 100.0f },
                                    { sp (s, "position"), 0.0f }, { sp (s, "pitch"), s % 2 ? 12.0f : -12.0f }, { sp (s, "filter_type"), 1.0f },
                                    { sp (s, "filter_res"), 100.0f }, { sp (s, "filter_cutoff"), 2000.0f }, { sp (s, "level"), 6.0f } };
                Job j; j.len = (juce::int64) (30.0 * kFs); j.block = 128; j.prepBlock = 128;
                j.params = cfg;
                constexpr juce::int64 se = (juce::int64) (10.0 * kFs);
                j.stim = [] (juce::int64 n, int) { return n < se ? (float) (0.5 * std::sin (2.0 * eg::kPi * 220.0 * (double) n / kFs) + 0.1 * std::sin (2.0 * eg::kPi * 3311.0 * (double) n / kFs)) : 0.0f; };
                base.push_back ({ "Z7d", j });
            }
            std::vector<Job> jobs; std::vector<double> cost;
            std::vector<std::shared_ptr<AgcTrace>> tr;
            for (auto& b : base)
                for (int leg = 0; leg < 2; ++leg)
                {
                    Job j = b.second;
                    j.params = j.params + P { { "fb_saturation", 0.0f } };   // last write wins
                    if (leg) legacyAgc (j);
                    tr.push_back (std::make_shared<AgcTrace>());
                    attachAgc (j, tr.back());
                    jobs.push_back (j); cost.push_back ((double) j.len);
                }
            std::vector<std::unique_ptr<Out>> outs (jobs.size());
            int bad = 0, engaged = 0; juce::String where;
            eg22b::renderPool (jobs, cost, [&] (size_t i, Out& o)
            {
                outs[i] = std::make_unique<Out> (std::move (o));
                const size_t k = i ^ 1u;
                if (outs[k] != nullptr)
                {
                    const size_t lo = i & ~(size_t) 1;
                    const bool same = sameBits (*outs[lo], *outs[lo + 1]);
                    if (! same) { ++bad; if (where.isEmpty()) where = base[lo / 2].first; }
                    if (tr[lo]->gMin() < 1.0f) ++engaged;
                    outs[lo].reset(); outs[lo + 1].reset();
                }
            });
            row (bad == 0 && engaged > 0, "AG3 sat 0: FB1 / FB2 / FB3 (37) / FB3n / FB4 / LG1 hot / Z7d renders bitwise == legacy staging",
                 juce::String::formatted ("%d/%d identical, %d with the AGC engaged", (int) base.size() - bad, (int) base.size(), engaged)
                     + (where.isEmpty() ? juce::String() : " first diff " + where));
        }

        // AG4 — pumping at defaults + fb 50 % (sign-off 3: <= 1 dB p-p).
        {
            const int blk = 64;
            auto mkJob = [&] (std::function<float (juce::int64, int)> stim)
            {
                Job j; j.len = (juce::int64) (30.0 * kFs); j.block = blk; j.prepBlock = blk;
                j.params = P { { "feedback", 50.0f } };
                j.stim = std::move (stim);
                return j;
            };
            // (a) −6 dBFS-peak noise bursts: 250 ms on / 250 ms off.
            const float a6 = 0.5011872f;
            auto bursts = [a6] (juce::int64 i, int ch) { return ((i / 12000) % 2 == 0) ? eg::hashNoise (i, ch, a6) : 0.0f; };
            const DrumLoop loop (kFs);
            std::vector<Job> jobs { mkJob (bursts), mkJob (loop.stim()) };
            std::vector<double> cost { 1.0, 1.0 };
            std::vector<std::shared_ptr<AgcTrace>> tr { std::make_shared<AgcTrace>(), std::make_shared<AgcTrace>() };
            attachAgc (jobs[0], tr[0]); attachAgc (jobs[1], tr[1]);
            eg22b::renderPool (jobs, cost, [] (size_t, Out&) {});
            const int perWin = (int) std::lround (0.4 * kFs / blk);
            const size_t skip = (size_t) (2.0 * kFs / blk);
            const double depthA = agcDepthDb (*tr[0], perWin, skip), depthB = agcDepthDb (*tr[1], perWin, skip);
            row (tr[0]->gMin() == 1.0f, "AG4a defaults + fb 50 %, -6 dBFS-peak noise bursts, 30 s: g_agc == 1",
                 juce::String::formatted ("min g %.6f, depth %.3f dB", (double) tr[0]->gMin(), depthA));
            row (depthB <= 1.0, "AG4b defaults + fb 50 %, 0 dBFS-peak kick/snare loop, 30 s: g_agc depth <= 1.0 dB p-p (400 ms windows)",
                 juce::String::formatted ("min g %.4f (%.2f dB), depth %.3f dB p-p", (double) tr[1]->gMin(), 20.0 * std::log10 ((double) tr[1]->gMin()), depthB));
        }

        // AG5 — Z7d decay at default sat (340 s, new vs legacy) + a freeze render bitwise.
        {
            P cfg { { "mix", 100.0f }, { "grain_limit", 3.0f }, { "buffer_length", 30.0f }, { "feedback", 100.0f }, { "fb_diffusion", 100.0f },
                    { "reverb_mix", 100.0f }, { "reverb_size", 100.0f }, { "reverb_damp", 0.0f } };
            for (int s = 1; s <= 4; ++s)
                cfg = cfg + P { { sp (s, "enable"), 1.0f }, { sp (s, "density"), 200.0f }, { sp (s, "size"), 2000.0f }, { sp (s, "spray"), 100.0f },
                                { sp (s, "position"), 0.0f }, { sp (s, "pitch"), s % 2 ? 12.0f : -12.0f }, { sp (s, "filter_type"), 1.0f },
                                { sp (s, "filter_res"), 100.0f }, { sp (s, "filter_cutoff"), 2000.0f }, { sp (s, "level"), 6.0f } };
            double last[2] { -1.0, -1.0 };
            float gmin[2] { 1.0f, 1.0f };
            auto run = [&] (int leg)
            {
                const int block = 128;
                auto p = make();
                eg::setAll (*p, cfg);
                p->getEngineForTest().hooks.agcLegacy = leg == 1;
                p->setSeed (0x5EEDF00Dull);   // both arms the same grain draws (make() seeds from the system)
                p->prepareToPlay (kFs, block);
                juce::AudioBuffer<float> buf (2, block);
                juce::MidiBuffer midi;
                const juce::int64 stimEnd = (juce::int64) (10.0 * kFs), end = (juce::int64) (340.0 * kFs);
                for (juce::int64 pos = 0; pos < end; pos += block)
                {
                    for (int c = 0; c < 2; ++c)
                        for (int i = 0; i < block; ++i)
                        {
                            const juce::int64 n = pos + i;
                            buf.setSample (c, i, n < stimEnd ? (float) (0.5 * std::sin (2.0 * eg::kPi * 220.0 * (double) n / kFs) + 0.1 * std::sin (2.0 * eg::kPi * 3311.0 * (double) n / kFs)) : 0.0f);
                        }
                    p->processBlock (buf, midi);
                    gmin[leg] = std::min (gmin[leg], p->getEngineForTest().loopAgc());
                    for (int c = 0; c < 2; ++c)
                        for (int i = 0; i < block; ++i)
                            if (buf.getSample (c, i) != 0.0f) last[leg] = (double) (pos + i) / kFs;
                }
            };
            std::thread t0 ([&] { run (0); }), t1 ([&] { run (1); });
            t0.join(); t1.join();
            row (last[0] >= 0.0 && last[0] < 329.0 && last[0] <= last[1],
                 "AG5a Z7d config at sat 20 %: exact zero before 329 s, no later than the legacy staging",
                 juce::String::formatted ("last non-zero %.2f s (legacy %.2f s); min g %.6f (legacy %.6f)", last[0], last[1], (double) gmin[0], (double) gmin[1]));

            // Freeze at default sat: fb 50 %, engage at 2 s, 10 s.
            auto fz = [] (bool leg)
            {
                Job j; j.len = (juce::int64) (10.0 * kFs); j.block = 512;
                j.params = eg26::z7hBase();
                j.stim = eg::sineStim (220.0, kFs, 0.5);
                j.events.push_back ({ (juce::int64) (2.0 * kFs), [] (Proc& p) { eg::setP (p, "freeze", 1.0f); } });
                j.events.push_back ({ (juce::int64) (7.0 * kFs), [] (Proc& p) { eg::setP (p, "freeze", 0.0f); } });
                if (leg) legacyAgc (j);
                return eg::render (j);
            };
            const auto a = fz (false), b = fz (true);
            const auto& d = a.eng().diag;
            row (sameBits (a, b) && d.freezeEngages >= 1, "AG5b freeze render (z7h base, fb 50 %, sat 20 %, freeze 2-7 s): bitwise == legacy staging",
                 juce::String::formatted ("engages %lld, %d diffs", (long long) d.freezeEngages, eg::diff (a, b).count));
        }
    }
} }

static void engineRows4()
{
    eg4::agRows();
}
