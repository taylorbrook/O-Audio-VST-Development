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
// Stage 3 / Phase 3.2 rows (PLAN-3.2 P120). The buffer strip's transport and
// derivations, gated through the processor surface the editor calls
// (readTelemetry) plus the engine's snapshot / peak ring:
//
//   TB1  triple buffer under a 10 kHz writer + spinning reader: no torn frame
//        (TB1n: a single-buffer variant tears)
//   WV1  impulse train → peak bins at the expected delays ± 1 bin, 4 rates
//        (WV1n: binning at the LIVE head, 512 block, 1 s span, misses)
//   WV2  D31 map: silence 0, −30 dBFS 0.375 ± 0.01, 0 dBFS 1, −60 dBFS 0
//   WV3  frozen spanSec == min (span, heldCaptured) / fs; early freeze short
//   WV4  frozen wave bitwise identical over 50 publishes; live wave scrolls in
//        whole bins (WV4n: fractional head-anchored bins do not)
//   GD1  dot delay == (W − readAbs (spawn log)) / fs within 1e-6 s; stream-1
//        dots present, 1-based (GD1n: a 0-based copy drops stream 1)
//   GD2  spread 0 / gain_rand 0 → pan 0, gain 1 exactly; spread 1 / gain_rand
//        → inverse of panGains; pitch +7 → 7; env == the window; stream pan
//   CN1  Σ counts == pool census, ≤ limit, limit == grain_limit (4 caps)
//        (CN1n: counting killing / fade slots exceeds the lowered limit)
//   BD1  LFO → s1_position: band == gp after the tick, ≠ base; unrouted == base
//   M8   0 allocations from block 0 with publish + peak writes (M8L liveness)
//   TL32 replaces TL31 (RESEARCH-3.2 R126)
//   FB1  INFO frozen band vs the measured spawn region (R124)
//   PL1  INFO widest frame bytes; readTelemetry cost @ 48 k / 192 k
//==============================================================================
namespace { namespace eg32
{
    using namespace eg;
    using eg25b::operator+;
    namespace D = ogran::ModDest;
    using Tel = GranulatorProcessor::Telemetry;
    constexpr int kBins = Tel::kWaveBins;

    /** One block of `n` samples from `stim` at absolute position `pos`. */
    inline void block (Proc& p, juce::AudioBuffer<float>& buf, juce::int64 pos, int n,
                       const std::function<float (juce::int64, int)>& stim)
    {
        buf.setSize (2, n, false, false, true);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < n; ++i)
                buf.setSample (ch, i, stim (pos + i, ch));
        juce::MidiBuffer midi;
        p.processBlock (buf, midi);
    }

    /** A bare instance: streams off, no feedback, the given span. */
    inline std::unique_ptr<Proc> quiet (double fs, float spanS, int prepBlock = 512)
    {
        auto p = make();
        setAll (*p, { { "s1_enable", 0.0f }, { "feedback", 0.0f }, { "buffer_length", spanS } });
        p->prepareToPlay (fs, prepBlock);
        return p;
    }

    /** Pool census (independent of the snapshot code): live, non-killing slots per
        stream — the Spawner::acquire pass-1 definition (kill fade or pending kill). */
    inline std::array<int, 4> census (const ogran::Engine& e)
    {
        std::array<int, 4> c {};
        const auto& pool = e.getPool();
        const auto& g = pool.getGrains();
        for (int i = 0; i < ogran::kPoolLive; ++i)
            if (pool.isActive (i) && ! (g[(size_t) i].killStep > 0.0f) && ! e.getSpawnerForTest().isPendingKill (i))
                ++c[(size_t) juce::jlimit (0, 3, g[(size_t) i].stream)];
        return c;
    }

    //==========================================================================
    // TB1 / TB1n
    //==========================================================================
    struct SingleBuffer   // the TB1n control: one frame, written and read in place
    {
        ogran::UiSnapshot buf {};
        ogran::UiSnapshot& writeBuffer() noexcept { return buf; }
        void publish() noexcept { std::atomic_thread_fence (std::memory_order_release); }
        const ogran::UiSnapshot& read() noexcept { std::atomic_thread_fence (std::memory_order_acquire); return buf; }
    };

    template <class TB>
    inline void tbStress (TB& tb, long long& frames, long long& torn, long long& backwards)
    {
        std::atomic<bool> stop { false };
        std::thread writer ([&tb, &stop]
        {
            std::uint64_t k = 0;
            auto next = std::chrono::steady_clock::now();
            while (! stop.load (std::memory_order_relaxed))
            {
                auto& s = tb.writeBuffer();
                ++k;
                reinterpret_cast<volatile std::uint64_t&> (s.seqHead) = k;
                for (int i = 0; i < ogran::UiSnapshot::kMaxGrains; ++i)
                    reinterpret_cast<volatile double&> (s.grains[i].readAbs) = (double) k;
                s.count = ogran::UiSnapshot::kMaxGrains;
                reinterpret_cast<volatile std::uint64_t&> (s.seqTail) = k;
                tb.publish();
                next += std::chrono::microseconds (100);   // 10 kHz
                while (std::chrono::steady_clock::now() < next && ! stop.load (std::memory_order_relaxed)) {}
            }
        });
        frames = torn = backwards = 0;
        std::uint64_t last = 0;
        const auto end = std::chrono::steady_clock::now() + std::chrono::seconds (1);
        while (std::chrono::steady_clock::now() < end)
        {
            const auto& s = tb.read();
            const std::uint64_t h = reinterpret_cast<const volatile std::uint64_t&> (s.seqHead);
            bool bad = false;
            for (int i = 0; i < ogran::UiSnapshot::kMaxGrains && ! bad; i += 17)
                bad = reinterpret_cast<const volatile double&> (s.grains[i].readAbs) != (double) h;
            const std::uint64_t t = reinterpret_cast<const volatile std::uint64_t&> (s.seqTail);
            ++frames;
            if (bad || h != t) ++torn;
            if (h < last) ++backwards;
            last = std::max (last, h);
        }
        stop = true;
        writer.join();
    }

    inline void tbRows()
    {
        std::printf ("\n-- TB. triple buffer (Stage 3 / 3.2, P114, R118)\n");
        long long f = 0, t = 0, b = 0;
        auto tb = std::make_unique<ogran::TripleBuffer<ogran::UiSnapshot>>();
        tbStress (*tb, f, t, b);
        row (f > 100000 && t == 0 && b == 0, "TB1 10 kHz writer x 1 s spinning reader: 0 torn frames, seq never goes back",
             juce::String::formatted ("%lld reads, %lld torn, %lld backwards", f, t, b));
        long long fn = 0, tn = 0, bn = 0;
        auto sb = std::make_unique<SingleBuffer>();
        tbStress (*sb, fn, tn, bn);
        row (tn > 0, "TB1n single-buffer variant tears (mismatches > 0)", juce::String::formatted ("%lld reads, %lld torn", fn, tn));
    }

    //==========================================================================
    // WV1–WV4
    //==========================================================================
    inline float impulseAt (juce::int64 i, const std::vector<juce::int64>& at)
    {
        return std::binary_search (at.begin(), at.end(), i) ? 1.0f : 0.0f;
    }

    inline void wvRows()
    {
        std::printf ("\n-- WV. waveform (Stage 3 / 3.2, P115, P117, R120-R122, D31)\n");

        // WV1 / WV1n — 1 s span, 6 impulses at known ring positions, 512 blocks.
        {
            Tally t;
            juce::String det;
            int nMiss = 0, nTot = 0;
            juce::int64 lagNc = 0;
            for (double fs : { 44100.0, 48000.0, 96000.0, 192000.0 })
            {
                const juce::int64 len = (juce::int64) (1.5 * fs) / 512 * 512;
                std::vector<juce::int64> at;
                for (double f : { 0.07, 0.19, 0.33, 0.52, 0.71, 0.93 })
                    at.push_back ((juce::int64) (0.5 * fs) + (juce::int64) (f * fs) - 7);
                const auto stim = [&at] (juce::int64 i, int) { return impulseAt (i, at); };
                auto p = quiet (fs, 1.0f);
                juce::AudioBuffer<float> buf (2, 512);
                for (juce::int64 pos = 0; pos < len; pos += 512) block (*p, buf, pos, 512, stim);
                // Make the snapshot lag the live head by one block (no publish at 512 < fs / 60).
                Tel tel;
                p->readTelemetry (tel);
                auto& e = p->getEngineForTest();
                std::int64_t snapHead = e.readUiSnapshot().head;
                if (snapHead == e.getWriteCount()) { block (*p, buf, len, 512, stim); p->readTelemetry (tel); snapHead = e.readUiSnapshot().head; }
                const std::int64_t live = e.getWriteCount();
                const double span = fs;   // 1 s
                std::array<float, kBins> ncWave {};
                ogran::binWave (e.getPeakRing(), live, span, ncWave.data(), kBins);
                int miss = 0, missNc = 0;
                for (auto i : at)
                {
                    const double kExp = kBins * (1.0 - (double) (snapHead - i) / span);
                    if (kExp < 1.0 || kExp > kBins - 2.0) continue;
                    const int k = (int) std::floor (kExp);
                    auto hit = [k] (const float* w) { for (int d = -1; d <= 1; ++d) if (w[juce::jlimit (0, kBins - 1, k + d)] > 0.999f) return true; return false; };
                    ++nTot;
                    if (! hit (tel.wave.data())) ++miss;
                    if (! hit (ncWave.data())) ++missNc;
                }
                int nonZero = 0;
                for (auto v : tel.wave) nonZero += v > 0.0f ? 1 : 0;
                t.check (miss == 0 && tel.waveCount == kBins && nonZero <= 3 * (int) at.size(),
                         juce::String::formatted ("%.0f: miss %d nonZero %d", fs, miss, nonZero));
                det << juce::String::formatted ("%.1fk miss %d lit %d; ", fs / 1000.0, miss, nonZero);
                if (juce::exactlyEqual (fs, 48000.0)) { nMiss = missNc; lagNc = live - snapHead; }
            }
            row (t.bad == 0 && nTot >= 20, "WV1 impulses at known ring positions -> peak bins at the expected delays +-1 bin (44.1 / 48 / 96 / 192 k)",
                 det.trim());
            row (nMiss > 0 && lagNc == 512, "WV1n binning at the LIVE head (512 block, 1 s span) misses +-1 bin",
                 juce::String::formatted ("48 k: %d of 6 missed, live - snapshot head %lld", nMiss, (long long) lagNc));
        }

        // WV2 — D31 map.
        {
            const double fs = 48000.0;
            auto level = [fs] (double dbfs, float& mean, float& lo, float& hi)
            {
                const double amp = dbfs <= -900.0 ? 0.0 : std::pow (10.0, dbfs / 20.0);
                auto p = quiet (fs, 1.0f);
                juce::AudioBuffer<float> buf (2, 1024);
                const auto stim = sineStim (1000.0, fs, amp);
                for (juce::int64 pos = 0; pos < (juce::int64) (1.3 * fs); pos += 1024) block (*p, buf, pos, 1024, stim);
                Tel tel;
                p->readTelemetry (tel);
                double s = 0.0; lo = 2.0f; hi = -1.0f;
                for (int k = 2; k < kBins - 2; ++k) { s += tel.wave[(size_t) k]; lo = juce::jmin (lo, tel.wave[(size_t) k]); hi = juce::jmax (hi, tel.wave[(size_t) k]); }
                mean = (float) (s / (kBins - 4));
            };
            float m0, l0, h0, m30, l30, h30, mF, lF, hF, m60, l60, h60;
            level (-999.0, m0, l0, h0);
            level (-30.0, m30, l30, h30);
            level (0.0, mF, lF, hF);
            level (-60.0, m60, l60, h60);
            row (h0 == 0.0f && std::abs (l30 - 0.375f) <= 0.01f && std::abs (h30 - 0.375f) <= 0.01f && lF >= 0.999f && h60 == 0.0f,
                 "WV2 D31 map: silence 0; -30 dBFS 0.375 +-0.01; 0 dBFS 1; -60 dBFS 0",
                 juce::String::formatted ("silence max %.3f; -30 [%.4f, %.4f]; 0 dBFS min %.4f; -60 max %.3f", h0, l30, h30, lF, h60));
        }

        // WV3 — frozen span: early freeze (2 s captured, 8 s span) and late (12 s, 4 s span).
        {
            const double fs = 48000.0;
            Tally t;
            juce::String det;
            for (int c = 0; c < 2; ++c)
            {
                const float span = c == 0 ? 8.0f : 4.0f;
                const juce::int64 fzAt = (juce::int64) ((c == 0 ? 2.0 : 12.0) * fs);
                auto p = quiet (fs, span);
                juce::AudioBuffer<float> buf (2, 1024);
                const auto stim = noiseStim (0.3f);
                juce::int64 pos = 0;
                for (; pos < fzAt; pos += 1024) block (*p, buf, pos, 1024, stim);
                setP (*p, "freeze", 1.0f);
                for (juce::int64 end = pos + (juce::int64) (0.5 * fs); pos < end; pos += 1024) block (*p, buf, pos, 1024, stim);
                Tel tel;
                p->readTelemetry (tel);
                const auto& fz = p->getEngineForTest().getFreezeForTest();
                const double want = std::min ((double) span * fs, (double) fz.heldCaptured) / fs;
                const bool ok = tel.frozen && std::abs (tel.spanSec - (float) want) <= 1e-6f * (float) want
                                && (c == 0 ? want < 2.5 : std::abs (tel.spanSec - span) <= 1e-5f);
                t.check (ok, juce::String::formatted ("case %d spanSec %.6f want %.6f frozen %d", c, tel.spanSec, want, (int) tel.frozen));
                det << juce::String::formatted ("%s: spanSec %.4f (buffer %.0f s, held %.4f s); ", c == 0 ? "early" : "late", tel.spanSec, span,
                                                (double) fz.heldCaptured / fs);
            }
            row (t.bad == 0, "WV3 frozen spanSec == min (span, heldCaptured) / fs; early freeze gives the short span", t.bad ? t.detail (2) : det.trim());
        }

        // WV4 / WV4n — frozen frames identical; live whole-bin scroll (absolute-cell anchoring).
        {
            const double fs = 48000.0;
            auto p = quiet (fs, 1.0f);
            juce::AudioBuffer<float> buf (2, 1024);
            const auto stim = noiseStim (0.4f);
            juce::int64 pos = 0;
            for (; pos < (juce::int64) (1.2 * fs); pos += 1024) block (*p, buf, pos, 1024, stim);

            // Live: 50 consecutive publishes (1024 >= fs / 60, every block publishes).
            auto& e = p->getEngineForTest();
            int shiftOk = 0, shiftOkNc = 0, pairs = 0;
            std::array<float, kBins> prev {}, prevNc {}, nc {};
            Tel tel;
            for (int f = 0; f < 51; ++f)
            {
                block (*p, buf, pos, 1024, stim); pos += 1024;
                p->readTelemetry (tel);
                const auto& snap = e.readUiSnapshot();
                ogran::binWave (e.getPeakRing(), snap.head, snap.spanSamples, nc.data(), kBins, true);
                if (f > 0)
                {
                    ++pairs;
                    auto scrolls = [] (const std::array<float, kBins>& a, const std::array<float, kBins>& b)
                    {
                        for (int m = 0; m <= 60; ++m)
                        {
                            bool same = true;
                            for (int k = 1; k + m < kBins - 2 && same; ++k)
                                same = std::memcmp (&b[(size_t) k], &a[(size_t) (k + m)], sizeof (float)) == 0;
                            if (same) return true;
                        }
                        return false;
                    };
                    shiftOk   += scrolls (prev, tel.wave) ? 1 : 0;
                    shiftOkNc += scrolls (prevNc, nc) ? 1 : 0;
                }
                prev = tel.wave;
                prevNc = nc;
            }

            // Frozen: 50 consecutive publishes after the stop.
            setP (*p, "freeze", 1.0f);
            for (int k = 0; k < 20; ++k) { block (*p, buf, pos, 1024, stim); pos += 1024; }
            p->readTelemetry (tel);
            const auto first = tel.wave;
            int same = 0;
            std::int64_t h0 = e.readUiSnapshot().head, hN = 0;
            for (int f = 0; f < 50; ++f)
            {
                block (*p, buf, pos, 1024, stim); pos += 1024;
                p->readTelemetry (tel);
                hN = e.readUiSnapshot().head;
                same += std::memcmp (first.data(), tel.wave.data(), sizeof (float) * kBins) == 0 ? 1 : 0;
            }
            int lit = 0;
            for (auto v : first) lit += v > 0.0f ? 1 : 0;
            row (tel.frozen && same == 50 && h0 == hN && lit > kBins / 2 && shiftOk == pairs,
                 "WV4 frozen: wave bitwise identical over 50 publishes; live: every frame is a whole-bin scroll of the previous",
                 juce::String::formatted ("frozen %d/50 identical (head %lld, %d bins lit); live %d/%d whole-bin scrolls", same, (long long) hN, lit, shiftOk, pairs));
            row (shiftOkNc < pairs / 2, "WV4n fractional head-anchored bins (R122 control) are not whole-bin scrolls",
                 juce::String::formatted ("%d/%d frames scroll", shiftOkNc, pairs));
        }
    }

    //==========================================================================
    // GD1 / GD2
    //==========================================================================
    /** A 4-stream render ending on a publishing block, so the snapshot IS the pool at the end. */
    inline std::unique_ptr<Proc> grainRender (const P& params, double fs, double seconds, std::uint64_t seed = 0x32D0ull)
    {
        auto p = make();
        setAll (*p, P { { "feedback", 0.0f }, { "buffer_length", 2.0f } } + params);
        p->setSeed (seed);
        p->prepareToPlay (fs, 1024);
        juce::AudioBuffer<float> buf (2, 1024);
        const auto stim = noiseStim (0.3f);
        for (juce::int64 pos = 0; pos < (juce::int64) (seconds * fs); pos += 1024) block (*p, buf, pos, 1024, stim);
        return p;
    }

    inline P fourStreams (float density = 40.0f)
    {
        P ps;
        for (int s = 1; s <= 4; ++s)
        {
            const juce::String n = "s" + juce::String (s) + "_";
            ps.push_back ({ n + "enable", 1.0f });
            ps.push_back ({ n + "density", density });
            ps.push_back ({ n + "size", 200.0f });
            ps.push_back ({ n + "position", 20.0f * (float) s });
            ps.push_back ({ n + "spray", 20.0f });
        }
        return ps;
    }

    inline void gdRows()
    {
        std::printf ("\n-- GD. grain dots (Stage 3 / 3.2, P116, P117, R119, R125)\n");
        const double fs = 48000.0;

        // GD1 / GD1n — delay vs the spawn log; stream-1 dots 1-based.
        {
            auto p = grainRender (fourStreams() + P { { "s1_reverse_prob", 30.0f }, { "s2_pitch", -5.0f } }, fs, 2.5);
            auto& e = p->getEngineForTest();
            Tel tel;
            p->readTelemetry (tel);
            const auto& snap = e.readUiSnapshot();
            const auto& pool = e.getPool();
            const auto& gs = pool.getGrains();
            const std::int64_t W = e.getWriteCount();
            // newest spawn record per slot
            std::map<int, const ogran::Engine::SpawnRecord*> bySlot;
            const auto& d = e.diag;
            const std::uint64_t first = d.logCount > (std::uint64_t) ogran::Engine::kSpawnLogSize ? d.logCount - ogran::Engine::kSpawnLogSize : 0;
            for (std::uint64_t k = first; k < d.logCount; ++k) bySlot[d.record (k).slot] = &d.record (k);
            int n = 0, checked = 0, bad = 0, s1 = 0, outOfRange = 0;
            double worst = 0.0;
            for (int i = 0; i < ogran::kPoolLive + ogran::kPoolFade && n < tel.grainCount; ++i)
            {
                if (! pool.isActive (i)) continue;
                const auto& dot = tel.grains[(size_t) n++];
                if (dot.stream < 1 || dot.stream > 4) ++outOfRange;
                s1 += dot.stream == 1 ? 1 : 0;
                if (i >= ogran::kPoolLive) continue;
                auto it = bySlot.find (i);
                if (it == bySlot.end()) continue;
                const auto& r = *it->second;
                const auto& g = gs[(size_t) i];
                const double readAbs = r.anchor + r.dir * r.rate * ((double) g.age + r.frac);   // the exact reference
                const double want = ((double) W - readAbs) / fs;
                const double err = std::abs ((double) dot.delaySec - want);
                worst = juce::jmax (worst, err);
                ++checked;
                if (err > 1e-6 || dot.stream != (std::uint8_t) (r.stream + 1)) ++bad;
            }
            int visibleNc = 0, visible1Nc = 0;
            for (int i = 0; i < tel.grainCount; ++i)
            {
                const int nn = tel.grains[(size_t) i].stream - 1;
                if (nn >= 1 && nn <= 4) { ++visibleNc; }
                if (nn >= 1 && nn <= 4 && tel.grains[(size_t) i].stream == 1) ++visible1Nc;
            }
            row (snap.head == W && checked >= 20 && bad == 0 && s1 > 0 && outOfRange == 0 && n == tel.grainCount,
                 "GD1 dot delay == (W - readAbs (spawn log)) / fs within 1e-6 s; stream-1 dots present, streams 1-based",
                 juce::String::formatted ("%d dots, %d checked vs the log, worst %.2e s, stream-1 dots %d", tel.grainCount, checked, worst, s1));
            row (s1 > 0 && visible1Nc == 0, "GD1n a 0-based stream copy drops every stream-1 dot (page skips n < 1)",
                 juce::String::formatted ("stream-1 dots %d -> visible %d (of %d visible)", s1, visible1Nc, visibleNc));
        }

        // GD2 — field recovery.
        {
            Tally t;
            juce::String det;
            // (a) spread 0 / gain_rand 0 / pitch +7 / Hann: pan 0, gain 1 exactly; pitch 7; env == Hann(phase).
            {
                P ps = fourStreams();
                for (int s = 1; s <= 4; ++s) { const juce::String n = "s" + juce::String (s) + "_"; ps.push_back ({ n + "stereo_spread", 0.0f }); ps.push_back ({ n + "pitch", 7.0f }); }
                auto p = grainRender (ps, fs, 1.5);
                auto& e = p->getEngineForTest();
                Tel tel;
                p->readTelemetry (tel);
                const auto& snap = e.readUiSnapshot();
                int panBad = 0, gainBad = 0, pitchBad = 0, envBad = 0;
                double envWorst = 0.0;
                for (int i = 0; i < tel.grainCount; ++i)
                {
                    const auto& dot = tel.grains[(size_t) i];
                    const auto& u = snap.grains[i];
                    panBad  += dot.pan == 0.0f ? 0 : 1;
                    gainBad += dot.gain == 1.0f ? 0 : 1;
                    pitchBad += std::abs (dot.pitchSt - 7.0f) <= 1e-4f ? 0 : 1;
                    const double hann = 0.5 - 0.5 * std::cos (2.0 * kPi * u.phase);
                    const double err = std::abs ((double) dot.env - hann * u.killGain);
                    envWorst = juce::jmax (envWorst, err);
                    envBad += err <= 1e-6 ? 0 : 1;
                }
                t.check (tel.grainCount >= 20 && panBad + gainBad + pitchBad + envBad == 0,
                         juce::String::formatted ("a: n %d pan %d gain %d pitch %d env %d", tel.grainCount, panBad, gainBad, pitchBad, envBad));
                det << juce::String::formatted ("a: %d dots pan 0 / gain 1 / pitch 7 exact, Hann env worst %.1e; ", tel.grainCount, envWorst);
            }
            // (b) spread 100 / gain_rand 50 / window 0.6 skew 30: pan in [-1, 1] and inverts panGains;
            //     gain in [-12 dB, 1]; env == the bank window from the PARAMETERS.
            {
                P ps = fourStreams();
                for (int s = 1; s <= 4; ++s)
                {
                    const juce::String n = "s" + juce::String (s) + "_";
                    ps.push_back ({ n + "stereo_spread", 100.0f }); ps.push_back ({ n + "gain_rand", 50.0f });
                    ps.push_back ({ n + "window", 0.6f }); ps.push_back ({ n + "window_skew", 30.0f });
                }
                auto p = grainRender (ps, fs, 1.5);
                auto& e = p->getEngineForTest();
                Tel tel;
                p->readTelemetry (tel);
                const auto& snap = e.readUiSnapshot();
                const int step = ogran::WindowMorphBank::stepFor (0.6f);
                const auto tilt = ouaricon::grain::WindowLut::makeTilt (0.5f + 0.45f * 0.3f);
                int bad = 0, spreadSeen = 0, cutSeen = 0;
                double invWorst = 0.0, envWorst = 0.0;
                const float floorG = std::pow (10.0f, -12.0f / 20.0f);
                for (int i = 0; i < tel.grainCount; ++i)
                {
                    const auto& dot = tel.grains[(size_t) i];
                    const auto& u = snap.grains[i];
                    bad += (dot.pan >= -1.0f && dot.pan <= 1.0f && dot.gain >= floorG - 1e-5f && dot.gain <= 1.0f + 1e-6f) ? 0 : 1;
                    spreadSeen += std::abs (dot.pan) > 0.2f ? 1 : 0;
                    cutSeen += dot.gain < 0.9f ? 1 : 0;
                    const auto pg = ogran::panGains (dot.pan);
                    invWorst = juce::jmax (invWorst, (double) juce::jmax (std::abs (pg.a * dot.gain - u.gainL), std::abs (pg.b * dot.gain - u.gainR)));
                    if (u.killing) continue;
                    const double w = ogran::WindowMorphBank::read (e.getBank().table (step), { tilt.t, tilt.a, tilt.b }, u.phase) * u.killGain;
                    envWorst = juce::jmax (envWorst, std::abs ((double) dot.env - w));
                }
                t.check (tel.grainCount >= 20 && bad == 0 && spreadSeen > 0 && cutSeen > 0 && invWorst <= 1e-5 && envWorst <= 1e-6,
                         juce::String::formatted ("b: bad %d spread %d cut %d inv %.1e env %.1e", bad, spreadSeen, cutSeen, invWorst, envWorst));
                det << juce::String::formatted ("b: panGains inverse worst %.1e, %d spread / %d cut dots, bank env worst %.1e; ", invWorst, spreadSeen, cutSeen, envWorst);
            }
            // (c) hard-left stream pan, spread 0: every stream-1 dot at pan -1 (sign-off 2).
            {
                P ps = fourStreams();
                for (int s = 1; s <= 4; ++s) ps.push_back ({ "s" + juce::String (s) + "_stereo_spread", 0.0f });
                ps.push_back ({ "s1_pan", -100.0f });
                auto p = grainRender (ps, fs, 1.5);
                Tel tel;
                p->readTelemetry (tel);
                int s1 = 0, left = 0, others = 0;
                for (int i = 0; i < tel.grainCount; ++i)
                {
                    const auto& dot = tel.grains[(size_t) i];
                    if (dot.stream == 1) { ++s1; left += dot.pan == -1.0f ? 1 : 0; }
                    else others += dot.pan == 0.0f ? 0 : 1;
                }
                t.check (s1 > 0 && left == s1 && others == 0, juce::String::formatted ("c: s1 %d left %d others off-centre %d", s1, left, others));
                det << juce::String::formatted ("c: %d/%d stream-1 dots at pan -1", left, s1);
            }
            row (t.bad == 0, "GD2 spread 0 -> pan 0 / gain 1 exact; spread + gain_rand invert panGains; pitch +7; env == window; stream pan",
                 t.bad ? t.detail (3) : det.trim());
        }
    }

    //==========================================================================
    // CN1 / CN1n
    //==========================================================================
    inline void cnRows()
    {
        std::printf ("\n-- CN. counts / limit (Stage 3 / 3.2, D32)\n");
        struct Scan { int frames = 0, mismatch = 0, over = 0, maxSum = 0, maxAll = 0, limitBad = 0, limit = 0; };
        auto mk = [] (int capIdx, bool lower, Scan* sc)
        {
            Job j;
            j.fs = 48000.0; j.len = (lower ? 188 : 94) * 1024; j.block = 1024;   // whole blocks: every block publishes j.prepBlock = 1024; j.seed = 0xC0A7ull;
            j.params = eg26::c7Params() + P { { "grain_limit", (float) capIdx } };
            j.stim = eg26::c7Stim (j.fs);
            // ≈ 3.0 s, on a block boundary: a split block (< fs / 60) would not publish.
            if (lower) j.events.push_back ({ 141 * 1024, [] (Proc& p) { setP (p, "grain_limit", 0.0f); } });
            j.after = [sc, lower] (Proc& p, juce::int64 pos)
            {
                Tel tel;
                p.readTelemetry (tel);
                const auto c = census (p.getEngineForTest());
                int sum = 0;
                for (int s = 0; s < 4; ++s) { sum += tel.counts[(size_t) s]; if (tel.counts[(size_t) s] != c[(size_t) s]) ++sc->mismatch; }
                ++sc->frames;
                const int cap = 32 * (1 + juce::roundToInt (p.parameters.getRawParameterValue ("grain_limit")->load()));
                if (tel.limit != cap) ++sc->limitBad;
                const bool settled = ! lower || pos >= (juce::int64) (3.25 * 48000);
                if (settled && sum > tel.limit) ++sc->over;
                if (settled) { sc->maxSum = juce::jmax (sc->maxSum, sum); sc->maxAll = juce::jmax (sc->maxAll, tel.grainCount); }
                sc->limit = tel.limit;
            };
            return j;
        };
        std::array<Scan, 5> sc {};
        std::vector<Job> jobs;
        for (int c = 0; c < 4; ++c) jobs.push_back (mk (c, false, &sc[(size_t) c]));
        jobs.push_back (mk (3, true, &sc[4]));
        eg25::renderAll (jobs);
        Tally t;
        juce::String det;
        for (int c = 0; c < 4; ++c)
        {
            const auto& s = sc[(size_t) c];
            t.check (s.frames > 50 && s.mismatch == 0 && s.over == 0 && s.limitBad == 0 && s.maxSum > 0,
                     juce::String::formatted ("cap %d: mismatch %d over %d limitBad %d", 32 * (c + 1), s.mismatch, s.over, s.limitBad));
            det << juce::String::formatted ("%d: max sum %d (dots %d); ", 32 * (c + 1), s.maxSum, s.maxAll);
        }
        const auto& L = sc[4];
        t.check (L.mismatch == 0 && L.over == 0 && L.limitBad == 0 && L.limit == 32, juce::String::formatted ("lowered: mismatch %d over %d limit %d", L.mismatch, L.over, L.limit));
        row (t.bad == 0, "CN1 sum(counts) == pool census every frame, <= limit, limit == grain_limit (32 / 64 / 96 / 128 + lowered 128 -> 32)",
             t.bad ? t.detail (5) : det.trim() + juce::String::formatted (" lowered: max sum %d", L.maxSum));
        row (L.maxAll > 32, "CN1n counting killing / fade slots exceeds the lowered limit (GL3 stimulus, 128 -> 32)",
             juce::String::formatted ("max dots incl. killing %d vs limit 32", L.maxAll));
    }

    //==========================================================================
    // BD1
    //==========================================================================
    inline void bdRows()
    {
        std::printf ("\n-- BD. modulated bands (Stage 3 / 3.2, D30, R123)\n");
        struct Scan { int frames = 0, gpBad = 0, moved = 0, unroutedBad = 0; std::vector<std::array<float, 6>> b; };
        auto mk = [] (bool route, Scan* sc)
        {
            Job j;
            j.fs = 48000.0; j.len = 94 * 1024; j.block = 1024; j.prepBlock = 1024;   // whole blocks: every block publishes
            j.params = fourStreams() + P { { "lfo1_rate", 3.0f } };
            if (route) j.params = j.params + P { { "mod1_source", 1.0f }, { "mod1_dest", (float) D::stream (0, D::Position) }, { "mod1_depth", 60.0f } };
            j.after = [sc] (Proc& p, juce::int64)
            {
                Tel tel;
                p.readTelemetry (tel);
                const auto& gp = p.getEngineForTest().getGridParams();
                ++sc->frames;
                for (int s = 0; s < 4; ++s)
                    if (std::memcmp (&tel.bands[(size_t) s][0], &gp.s[s].position, 4) != 0 || std::memcmp (&tel.bands[(size_t) s][1], &gp.s[s].spray, 4) != 0) ++sc->gpBad;
                const float base = p.parameters.getRawParameterValue ("s1_position")->load() / 100.0f;
                if (std::abs (tel.bands[0][0] - base) > 0.01f) ++sc->moved;
                sc->b.push_back ({ tel.bands[1][0], tel.bands[1][1], tel.bands[2][0], tel.bands[2][1], tel.bands[3][0], tel.bands[3][1] });
            };
            return j;
        };
        Scan r, u;
        eg25::renderAll ({ mk (true, &r), mk (false, &u) });
        bool unrouted = r.b.size() == u.b.size() && ! r.b.empty();
        for (size_t i = 0; unrouted && i < r.b.size(); ++i) unrouted = std::memcmp (r.b[i].data(), u.b[i].data(), sizeof (float) * 6) == 0;
        row (r.frames > 50 && r.gpBad == 0 && u.gpBad == 0 && r.moved > r.frames / 4 && u.moved == 0 && unrouted,
             "BD1 LFO -> s1_position: band == gp.s[0] after the tick and != base; streams 2-4 bitwise == the unrouted render",
             juce::String::formatted ("%d frames, gp mismatches %d / %d, s1 off base in %d (unrouted render %d), streams 2-4 identical %s",
                                      r.frames, r.gpBad, u.gpBad, r.moved, u.moved, unrouted ? "yes" : "NO"));
    }

    //==========================================================================
    // M8 — allocations from block 0 with publish + peak writes active
    //==========================================================================
    inline void m8Rows()
    {
        std::printf ("\n-- M8. allocations with the 3.2 telemetry active, armed from block 0 (P120)\n");
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
            j.fs = 48000.0; j.len = 6 * 48000; j.seed = 0xA778ull;
            j.block = sched == 0 ? 0 : 8192;
            j.prepBlock = 512;
            j.params = eg26::c7Params();
            j.stim = eg26::c7Stim (j.fs);
            j.events.push_back ({ 2 * 48000 + 13, [] (Proc& p) { setP (p, "freeze", 1.0f); } });
            j.events.push_back ({ 3 * 48000 + 7, [] (Proc& p) { setP (p, "freeze", 0.0f); } });
            auto reads = std::make_shared<int> (0), withGrains = std::make_shared<int> (0);
            j.after = [reads, withGrains] (Proc& p, juce::int64)   // message side, NOT armed
            {
                Tel tel;
                p.readTelemetry (tel);
                ++*reads;
                if (tel.hasBuffer && tel.grainCount > 0) ++*withGrains;
            };
            armDuringProcess = true;
            const auto o = render (j);
            armDuringProcess = false;
            total += allocCount;
            Tel tel;
            o.p->readTelemetry (tel);
            int lit = 0;
            for (auto v : tel.wave) lit += v > 0.0f ? 1 : 0;
            live = live && *withGrains > 0 && lit > 0;
            det << (sched == 0 ? "var" : "8192 (prep 512)") << juce::String::formatted (": %d allocs, %d reads (%d with grains), %d bins lit; ",
                                                                               (int) allocCount, *reads, *withGrains, lit);
            allocCount = 0;
        }
        malloc_logger = nullptr;
        allocTrace = false;
        row (liveness == 1, "M8L liveness: a deliberate malloc on the render thread counts exactly 1", juce::String (liveness));
        row (liveness == 1 && total == 0 && live, "M8 0 allocations from block 0 with snapshot publish + peak writes (var, fresh 8192 after 512, freeze edges)", det.trim());
       #else
        row (false, "M8 alloc gate is macOS-only");
       #endif
    }

    //==========================================================================
    // FB1 / PL1 — INFO
    //==========================================================================
    inline void infoRows()
    {
        std::printf ("\n-- FB1 / PL1. INFO (R124, P119)\n");
        // FB1: 2 s grains at a 30 s span, frozen: the band vs the spawn region actually used.
        {
            const double fs = 48000.0;
            auto p = make();
            setAll (*p, { { "feedback", 0.0f }, { "buffer_length", 30.0f }, { "s1_size", 2000.0f }, { "s1_density", 20.0f },
                          { "s1_position", 40.0f }, { "s1_spray", 20.0f }, { "s1_pitch", 12.0f } });
            p->prepareToPlay (fs, 4096);
            juce::AudioBuffer<float> buf (2, 4096);
            const auto stim = noiseStim (0.3f);
            juce::int64 pos = 0;
            for (; pos < (juce::int64) (31.0 * fs); pos += 4096) block (*p, buf, pos, 4096, stim);
            setP (*p, "freeze", 1.0f);
            auto& e = p->getEngineForTest();
            const std::uint64_t log0 = e.diag.logCount;
            for (juce::int64 end = pos + (juce::int64) (4.0 * fs); pos < end; pos += 4096) block (*p, buf, pos, 4096, stim);
            Tel tel;
            p->readTelemetry (tel);
            const auto& fz = e.getFreezeForTest();
            double lo = 1e9, hi = -1e9;
            int n = 0;
            for (std::uint64_t k = log0; k < e.diag.logCount; ++k)
            {
                const auto& r = e.diag.record (k);
                if (r.writeHead != fz.W0) continue;   // frozen spawns only
                const double f = r.delay / (tel.spanSec * fs);
                lo = juce::jmin (lo, f); hi = juce::jmax (hi, f); ++n;
            }
            const float a = tel.bands[0][0] - tel.bands[0][1] / 2, b = tel.bands[0][0] + tel.bands[0][1] / 2;
            std::printf ("  [INFO] FB1 frozen band [%.3f, %.3f] of span %.2f s vs measured spawn delays [%.3f, %.3f] (%d spawns): offset lo %+.3f hi %+.3f\n",
                         a, b, tel.spanSec, lo, hi, n, lo - a, hi - b);
        }
        // PL1: widest frame bytes (464 wave + 128 grains, full-precision doubles) + readTelemetry cost.
        {
            for (double fs : { 48000.0, 192000.0 })
            {
                auto p = make();
                setAll (*p, eg26::c7Params() + P { { "buffer_length", 30.0f } });
                p->prepareToPlay (fs, 2048);
                juce::AudioBuffer<float> buf (2, 2048);
                const auto stim = eg26::c7Stim (fs);
                for (juce::int64 pos = 0; pos < (juce::int64) (2.0 * fs); pos += 2048) block (*p, buf, pos, 2048, stim);
                Tel tel;
                const auto t0 = juce::Time::getHighResolutionTicks();
                for (int k = 0; k < 20; ++k) p->readTelemetry (tel);
                const double us = juce::Time::highResolutionTicksToSeconds (juce::Time::getHighResolutionTicks() - t0) / 20.0 * 1e6;
                const auto json = juce::JSON::toString (ogran::ui::telemetryToVar (tel), true);
                std::printf ("  [INFO] PL1 %.0f k: frame %d bytes (%d grains, %d wave bins), readTelemetry %.0f us (30 s span)\n",
                             fs / 1000.0, (int) json.getNumBytesAsUTF8(), tel.grainCount, tel.waveCount, us);
            }
        }
    }

    //==========================================================================
    // TL32 (replaces TL31, R126)
    //==========================================================================
    inline void tlRows()
    {
        std::printf ("\n-- TL. telemetry (Stage 3 / 3.2, replaces TL31)\n");
        {
            auto un = make();
            Tel t;
            const bool ok = un->readTelemetry (t);
            row (! ok && ! t.hasBuffer && ! t.hasCounts, "TL32u readTelemetry false only before prepare; has* false then");
        }
        auto p = make();
        p->prepareToPlay (48000.0, 256);
        setParam (*p, "mod3_source", 1.0f);  setParam (*p, "mod3_dest", 5.0f);
        setParam (*p, "mod7_source", 9.0f);  setParam (*p, "mod7_dest", 40.0f);
        setParam (*p, "lfo1_rate", 7.0f);
        setParam (*p, "macro2", 63.0f);
        juce::AudioBuffer<float> buf (2, 256);
        juce::MidiBuffer midi;
        juce::Random rng (31);
        for (int b = 0; b < 200; ++b)
        {
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 256; ++i)
                    buf.setSample (ch, i, 0.3f * std::sin (0.0577f * (float) (b * 256 + i)) + 0.01f * (rng.nextFloat() - 0.5f));
            p->processBlock (buf, midi);
        }
        Tel t;
        t.counts[0] = 99; t.loopGainDb = 1.0f; t.hasMeters = true;   // dirty: readTelemetry must clear
        const bool ok = p->readTelemetry (t);
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
        // 3.2 live
        int lit = 0, cnt = 0;
        for (auto v : t.wave) lit += v > 0.0f ? 1 : 0;
        for (auto c : t.counts) cnt += c;
        tl.check (t.hasBuffer && t.waveCount == kBins && lit > 0, "wave live");
        tl.check (t.grainCount > 0 && t.grainCount <= Tel::kMaxGrains, "grains live " + juce::String (t.grainCount));
        tl.check (juce::exactlyEqual (t.spanSec, 8.0f) && ! t.frozen, "span 8 s / not frozen " + juce::String (t.spanSec));
        tl.check (juce::exactlyEqual (t.bands[0][0], 0.0f) && juce::exactlyEqual (t.bands[0][1], 0.1f), "bands s1 base");
        tl.check (t.hasCounts && t.limit == 128 && cnt > 0 && cnt <= 128, "counts / limit " + juce::String (cnt) + " / " + juce::String (t.limit));
        // 3.3 still zero / NaN
        bool zero = ! t.hasMeters && ! t.hasOnset && ! t.hasLoopGain
                    && t.outPeakL == 0.0f && t.outPeakR == 0.0f && t.inPeak == 0.0f && t.onsetPulse == 0.0f && ! t.onsetFired
                    && std::isnan (t.loopGainDb);
        for (auto ph : t.lfoPhase) zero = zero && ph == 0.0f;
        tl.check (zero, "3.3 fields not zero / NaN");
        bool live = false;
        for (auto v : t.sources) live = live || v != 0.0f;
        tl.check (live, "liveness: every source reads 0 after 200 blocks");
        row (ok && tl.bad == 0, "TL32 readTelemetry: 3.1 fields == engine; 3.2 wave / grains / span / bands / counts live; 3.3 zero / NaN",
             tl.detail (11 + 5 + routed + 6) + "  grains " + juce::String (t.grainCount) + ", counts " + juce::String (cnt)
             + ", LFO1 " + juce::String (t.sources[0], 3));
    }
} } // namespace eg32 (anonymous)

static void engineRows32()
{
    using namespace eg32;
    tbRows();
    wvRows();
    gdRows();
    cnRows();
    bdRows();
    m8Rows();
    tlRows();
    infoRows();
}
