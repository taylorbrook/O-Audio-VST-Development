/*
  ==============================================================================

    O-Granulator — render harness, Stage 2 / Phase 2.5b f0 ring, follow,
    matrix and PERF rows (PLAN-2.5b P76). Sibling of engine_gates_25b.h.

      FR0      capacities 8–768 kHz; R69 lag constants                (T2)
      FR1–FR10 f0 history ring + follow (D17, R62–R65)                (T7, T8)
      AQ1–AQ8  absolute quantiser (D19, R70–R72)                       (T8)
      MX1–MX2  Pitch / Onset × class; removal (FUNC-06)                (T9)
      QB1–QB2  QUAL-01 bursts                                          (T9)
      N9 / M6 / Z6 / C6   PERF-03 / PERF-01 / QUAL-02 / PERF-02        (T10)

  ==============================================================================
*/
#pragma once

namespace { namespace eg25y
{
    using eg::Job;
    using eg::Out;
    using eg::Proc;
    using eg::P;
    using eg25::renderAll;
    using eg25b::route;
    using eg25b::operator+;
    using namespace eg25x;

    //==========================================================================
    // FR0 — capacities and constants (T2)
    //==========================================================================
    void capacityRows()
    {
        std::printf ("\n-- FR0. analysis capacities 8-768 kHz; R69 lag constants\n");
        Tally t;
        int n = 0;
        int worstN = 0, worstLog = 0, worstTau = 0;
        for (double fs : { 8000.0, 11025.0, 16000.0, 22050.0, 32000.0, 44100.0, 48000.0, 88200.0, 96000.0,
                           176400.0, 192000.0, 352800.0, 384000.0, 705600.0, 768000.0 })
        {
            const auto d = ogran::AnalysisDims::compute (fs);
            ++n;
            worstN = juce::jmax (worstN, d.N);
            worstLog = juce::jmax (worstLog, d.logLen);
            worstTau = juce::jmax (worstTau, d.tauMax);
            t.check (d.N <= ogran::kF0RingCap && d.logLen <= ogran::kCellLogCap && d.tauMax <= 1022 && d.tauMin >= 1,
                     fsName (fs) + juce::String::formatted (": N %d log %d tauMax %d", d.N, d.logLen, d.tauMax));
        }
        row (t.bad == 0, "FR0 N <= kF0RingCap, cell log <= kCellLogCap, tau_max <= 1022 at 15 rates",
             t.detail (n) + juce::String::formatted ("  (max N %d, log %d, tau %d)", worstN, worstLog, worstTau));

        // RESEARCH §3 table: window-centre lag 511.5·D + 31.5 and newest entry + 3·Hs.
        struct Lag { double fs; int D; double centre, newest; };
        const Lag table[] { { 44100.0, 4, 2077.5, 3613.5 }, { 48000.0, 4, 2077.5, 3613.5 }, { 88200.0, 7, 3612.0, 6300.0 },
                            { 96000.0, 8, 4123.5, 7195.5 }, { 192000.0, 16, 8215.5, 14359.5 } };
        Tally t2;
        for (const auto& L : table)
        {
            const auto d = ogran::AnalysisDims::compute (L.fs);
            t2.check (d.D == L.D && d.Hs == 128 * L.D && d.lagCentre == L.centre && d.lagNewest == L.newest,
                      fsName (L.fs) + juce::String::formatted (": D %d centre %.1f newest %.1f", d.D, d.lagCentre, d.lagNewest));
        }
        const auto d48 = ogran::AnalysisDims::compute (48000.0);
        row (t2.bad == 0, "FR0 R69 lags == RESEARCH s3 (2077.5 / 3613.5 ... 8215.5 / 14359.5)",
             t2.detail (5) + juce::String::formatted ("  48k: N %d, hold %d hops, log %d", d48.N, d48.hold, d48.logLen));
    }
    //==========================================================================
    // FR1–FR4 — the f0 history ring + cell log (D17 via R62 / R63 / R65) (T7)
    //==========================================================================
    void ringRows()
    {
        std::printf ("\n-- FR1-FR4. f0 history ring (tags, newest rule, freeze edge, alignment)\n");
        using M = ogran::F0Ring;
        // FR1 — tags.
        {
            auto r = std::make_unique<ogran::F0Ring>();
            r->prepare (16, 512);
            float f = 0.0f;
            const int never = r->lookup (5.0 * 512 + 3.0, false, f);
            r->writeSlot (5, 60.0f);
            const int hit5 = r->lookup (5.0 * 512 + 3.0, false, f);
            const float f5 = f;
            r->writeSlot (5 + 16, 61.0f);
            const int miss5 = r->lookup (5.0 * 512 + 3.0, false, f);
            const int hit21 = r->lookup (21.0 * 512 + 100.0, false, f);
            const float f21 = f;
            r->writeSlot (7, std::numeric_limits<float>::quiet_NaN());
            const int nan7 = r->lookup (7.0 * 512, false, f);
            row (never == M::Missing && hit5 == M::Hit && f5 == 60.0f && miss5 == M::Missing && hit21 == M::Hit && f21 == 61.0f && nan7 == M::Unconfident,
                 "FR1 tags: never-written -> miss; k then k + N -> k misses (tag), k + N hits; NaN entry -> unconfident",
                 juce::String::formatted ("%d %d %d %d %d", never, hit5, miss5, hit21, nan7));
        }
        // FR2 — newest rule.
        {
            auto r = std::make_unique<ogran::F0Ring>();
            r->prepare (64, 512);
            float f = 0.0f;
            const int empty = r->lookup (1000.0, false, f);
            r->writeSlot (10, 62.5f);
            r->writeSlot (9, 61.0f);
            f = 0.0f;
            const int ahead = r->lookup (40.0 * 512, false, f);
            const float fa = f;
            const int strict = r->lookup (40.0 * 512, true, f);
            row (empty == M::Missing && ahead == M::Hit && fa == 62.5f && strict == M::Missing && r->newest() == 10,
                 "FR2 newest rule: k > kNewest -> kNewest (R62); strictNewest -> miss; kNewest < 0 -> miss",
                 juce::String::formatted ("%d %d (%.1f) %d", empty, ahead, (double) fa, strict));
        }
        // FR3 — freeze edge on a synthetic cell log (Hs 8).
        {
            ogran::CellLog cells;
            cells.prepare (32);
            for (int c = 0; c < 10; ++c) cells.record (c * 32, c * 32, true);           // live: W = abs
            for (int c = 10; c < 14; ++c) cells.record (c * 32, 320, false);            // frozen at W0 = 320
            for (int c = 14; c < 18; ++c) cells.record (c * 32, 320 + (c - 14) * 32, true);   // released: W continues from W0
            auto r = std::make_unique<ogran::F0Ring>();
            r->prepare (256, 8);
            const int wLive = r->write (300.5, 50.0f, cells);    // last live cell (9): W = 288 + 12.5
            const int wFrz  = r->write (330.0, 51.0f, cells);    // first frozen cell (10): skipped
            const int wRel  = r->write (450.0, 52.0f, cells);    // cell 14 (448): W = 320 + 2 → slot 40
            float f = 0.0f;
            const int l37 = r->lookup (300.5, false, f);
            const float f37 = f;
            const int l40 = r->lookup (322.0, false, f);
            const float f40 = f;
            row (wLive == 0 && wFrz == 1 && wRel == 0 && l37 == M::Hit && f37 == 50.0f && l40 == M::Hit && f40 == 52.0f,
                 "FR3 freeze edge: the last live cell is written at W_cell + offset; the first frozen cell is skipped; after release W continues from W0",
                 juce::String::formatted ("write %d / %d / %d; slot 37 %.0f, slot 40 %.0f", wLive, wFrz, wRel, (double) f37, (double) f40));
        }
        // FR4 / FR4n — alignment (R69): 233 Hz, 311 Hz from S1 = 1 s to S2 = 2 s, 233 Hz again; never frozen.
        {
            const double fs = 48000.0, S1 = 1.0 * fs, S2 = 2.0 * fs;
            Job j = anaJob (fs, 3.0, fromBuffer (steppedSine (fs, 3.0, { { 0.0, 233.0 }, { 1.0, 311.0 }, { 2.0, 233.0 } }, 0.25)));
            auto H = hopsInto (j);
            renderAll ({ j });
            const auto dm = ogran::AnalysisDims::compute (fs);
            const double B = midiOf (311.0);
            double first = -1.0, last = -1.0;
            int nB = 0;
            for (const auto& h : H->h)
                if (h.decided >= 0 && h.ringResult == 0 && ! std::isnan (h.decidedNote) && std::abs ((double) h.decidedNote - B) < 0.5)
                {
                    if (first < 0.0) first = h.decidedCentre;
                    last = h.decidedCentre;
                    ++nB;
                }
            const double mid = 0.5 * (first + last), want = 0.5 * (S1 + S2);
            const double err = (mid - want) / dm.Hs;
            const double errN = (mid + 3.0 * dm.Hs - want) / dm.Hs;   // FR4n: timestamped at the decision hop j + 3
            row (nB > 50 && std::abs (err) <= 0.75 && errN >= 2.25, "FR4 the B region's centre in the ring within +-0.75 Hs of (S1 + S2)/2 (R69 constants); FR4n decision-hop stamps >= 2.25 Hs late",
                 juce::String::formatted ("%d B entries, centre error %+.3f Hs; decision-hop stamps %+.3f Hs", nB, err, errN));
        }
    }

    //==========================================================================
    // FR5–FR10, AQ1–AQ8 — follow (D17 / D19 / R62–R72) (T8)
    //==========================================================================
    inline int scaleChoice (const char* name)
    {
        const auto& n = GranulatorParamIds::scaleNames();
        for (int c = 0; c < n.size(); ++c) if (n[c] == name) return c;
        return -1;
    }

    /** The D17 recipe: s1 only, follow on, scale Off (D19), buffer 8 s, size 50 ms,
        spray 0, density 20 g/s, jitter 0, at a position (%). */
    inline Job followJob (double lenS, Stim st, float posPct, P extra = {})
    {
        Job j = anaJob (48000.0, lenS, std::move (st),
                        eg22::base ({ 1 }) + P { { "pitch_follow_s1", 1.0f }, { "buffer_length", 8.0f }, { "s1_size", 50.0f },
                                                 { "s1_spray", 0.0f }, { "s1_density", 20.0f }, { "s1_jitter", 0.0f },
                                                 { "s1_position", posPct } } + extra);
        return j;
    }
    inline void hook (Job& j, std::function<void (ogran::Engine::TestHooks&)> f)
    {
        auto prev = j.pre;
        j.pre = [prev, f] (Proc& p) { if (prev) prev (p); f (p.getEngineForTest().hooks); };
    }
    inline std::vector<const Rec*> inWindow (const eg22::RecLog& L, double t0, double t1, int stream = 0)
    {
        std::vector<const Rec*> v;
        for (const auto& r : L.r)
            if (r.stream == stream && (double) r.onsetSample >= t0 * 48000.0 && (double) r.onsetSample < t1 * 48000.0) v.push_back (&r);
        return v;
    }
    inline double ct (double a, double b) { return 100.0 * std::abs (a - b); }

    void followRows()
    {
        std::printf ("\n-- FR5-FR10. follow: content f0 (D17), frozen, unconfident gap, position 0, midpoint, two-pass draws\n");
        const double fs = 48000.0;
        const double A = midiOf (233.0), B = midiOf (311.0);
        auto ab = steppedSine (fs, 4.0, { { 0.0, 233.0 }, { 2.0, 311.0 } }, 0.25);
        // FR5 / FR5n (live), FR6 / FR6n (frozen at 1.5 s), positions.
        {
            std::vector<Job> jobs;
            std::vector<std::shared_ptr<eg22::RecLog>> logs;
            auto add = [&] (Job j) { logs.push_back (collectInto (j)); jobs.push_back (j); };
            add (followJob (4.0, fromBuffer (ab), 12.5f));                                    // 0: 1 s behind
            add (followJob (4.0, fromBuffer (ab), 0.0f));                                     // 1: position 0
            { Job j = followJob (4.0, fromBuffer (ab), 12.5f); hook (j, [] (auto& h) { h.liveF0Lookup = true; }); add (j); }   // 2: FR5n
            for (float pos : { 0.0f, 25.0f, 50.0f })                                            // 3–5: frozen
            {
                Job j = followJob (4.0, fromBuffer (ab), pos);
                j.events.push_back ({ (juce::int64) (1.5 * fs), [] (Proc& p) { eg::setP (p, "freeze", 1.0f); } });
                add (j);
            }
            { Job j = followJob (4.0, fromBuffer (ab), 25.0f); j.events.push_back ({ (juce::int64) (1.5 * fs), [] (Proc& p) { eg::setP (p, "freeze", 1.0f); } });
              hook (j, [] (auto& h) { h.liveF0Lookup = true; }); add (j); }                    // 6: FR6n
            auto o = renderAll (jobs);
            auto allNear = [] (const std::vector<const Rec*>& v, double note, double tolCt, int& n, int& hits)
            {
                n = (int) v.size(); hits = 0;
                bool ok = ! v.empty();
                for (auto* r : v) { const bool h = r->followHit && ct (r->f0, note) <= tolCt; hits += h ? 1 : 0; ok = ok && h; }
                return ok;
            };
            int n0, h0, n1, h1, n2, h2;
            const auto w0 = inWindow (*logs[0], 2.3, 2.9);
            bool noneB = true;
            for (auto* r : w0) noneB = noneB && ! (std::abs (r->f0 - B) < 1.0);
            const bool fr5 = allNear (w0, A, 10.0, n0, h0) && noneB;
            const bool fr5p = allNear (inWindow (*logs[1], 2.15, 4.0), B, 10.0, n1, h1);
            const bool fr5n = allNear (inWindow (*logs[2], 2.3, 2.9), B, 10.0, n2, h2);
            row (fr5 && fr5p, "FR5 live: grains in [2.3, 2.9] s at position 12.5 % (1 s behind) read A, none within 1 st of B; at position 0 they read B",
                 juce::String::formatted ("1 s behind %d/%d at A; position 0 %d/%d at B", h0, n0, h1, n1));
            row (fr5n, "FR5n liveF0Lookup (the ARCH s5 live-f0 form): the 1 s-behind grains read B", juce::String::formatted ("%d/%d at B", h2, n2));
            Tally t6;
            for (int k = 3; k <= 5; ++k)
            {
                int n, h;
                t6.check (allNear (inWindow (*logs[(size_t) k], 2.3, 2.9), A, 10.0, n, h) && o[(size_t) k].eng().diag.f0Skips > 0,
                          juce::String::formatted ("pos %d: %d/%d at A, skips %lld", (k - 3) * 25, h, n, (long long) o[(size_t) k].eng().diag.f0Skips));
            }
            int n6, h6;
            const bool fr6n = allNear (inWindow (*logs[6], 2.3, 2.9), B, 10.0, n6, h6);
            row (t6.bad == 0, "FR6 frozen at 1.5 s, input A -> B at 2 s: grains in [2.3, 2.9] s at positions 0 / 25 / 50 % read A; f0Skips > 0", t6.detail (3));
            row (fr6n, "FR6n liveF0Lookup while frozen: B", juce::String::formatted ("%d/%d at B", h6, n6));
        }
        // FR7 / FR7n — a 200 ms −20 dBFS white-noise gap (shorter than the hold).
        {
            auto x = std::make_shared<std::vector<float>> ((size_t) (4.0 * fs), 0.0f);
            auto a1 = steppedSine (fs, 4.0, { { 0.0, 233.0 } }, 0.25);
            for (size_t i = 0; i < x->size(); ++i)
            {
                const double t = (double) i / fs;
                (*x)[i] = (t >= 1.5 && t < 1.7) ? (float) (0.1 * gauss ((juce::int64) i, 71)) : (*a1)[i];
            }
            Job a = followJob (4.0, fromBuffer (x), 12.5f, P { { "s1_density", 100.0f } }), b = a;
            hook (b, [] (auto& h) { h.ringHold = true; });
            auto La = collectInto (a), Lb = collectInto (b);
            auto o = renderAll ({ a, b });
            const auto& ring = eg25x::an (o[0]).getRing();
            const auto& ringB = eg25x::an (o[1]).getRing();
            int overGap = 0, bad = 0, overGapB = 0, hitB = 0;
            for (size_t q = 0; q < La->r.size(); ++q)
            {
                const auto& r = La->r[q];
                const auto k = (std::int64_t) std::floor (r.lookupW / ring.hop());
                if (k < 0 || k > ring.newest()) continue;
                const auto& e = ring.entry ((int) (k % ring.size()));
                if (e.tag != k || ! std::isnan (e.note)) continue;
                ++overGap;
                const bool ok = r.followMiss == 1 && ! r.followHit && std::isnan (r.target) && r.pitchCents == 0.0f && r.rate == 1.0;
                bad += ok ? 0 : 1;
            }
            for (const auto& r : Lb->r)
            {
                const auto k = (std::int64_t) std::floor (r.lookupW / ringB.hop());
                if (k < 0) continue;
                const auto& e = ring.entry ((int) (k % ring.size()));   // the un-held ring says: this slot is a gap slot
                if (e.tag != k || ! std::isnan (e.note)) continue;
                ++overGapB;
                hitB += (r.followHit && ct (r.f0, A) <= 10.0) ? 1 : 0;
            }
            row (overGap >= 5 && bad == 0, "FR7 grains whose content midpoint lies over the gap's NaN entries: followMiss 1, interval mode (target NaN, the D4 pitch)",
                 juce::String::formatted ("%d over the gap, %d wrong", overGap, bad));
            row (overGapB >= 5 && hitB == overGapB, "FR7n ringHold (the R65 control): those grains hit with A", juce::String::formatted ("%d/%d hit A", hitB, overGapB));
        }
        // FR8 / FR8n — position 0 (R62).
        {
            Job a = followJob (3.0, eg::sineStim (227.0, fs, 0.25), 0.0f), b = a;
            hook (b, [] (auto& h) { h.strictNewest = true; });
            auto La = collectInto (a), Lb = collectInto (b);
            renderAll ({ a, b });
            const auto wa = inWindow (*La, 0.2, 3.0), wb = inWindow (*Lb, 0.2, 3.0);
            int ha = 0, hb = 0;
            for (auto* r : wa) ha += (r->followHit && ct (r->f0, midiOf (227.0)) <= 10.0) ? 1 : 0;
            for (auto* r : wb) hb += r->followHit ? 1 : 0;
            row (ha == (int) wa.size() && ! wa.empty() && hb == 0, "FR8 position 0: every grain after 0.2 s hits at 227 Hz (newest entry, R62); FR8n strictNewest -> 0 hits",
                 juce::String::formatted ("%d/%d; strict %d/%d", ha, (int) wa.size(), hb, (int) wb.size()));
        }
        // FR9 / FR9n — midpoint (R64): an 800 ms grain starting 300 ms before the A → B boundary.
        {
            Job a = followJob (4.0, fromBuffer (ab), 22.5f, P { { "s1_size", 800.0f }, { "s1_density", 2.0f } }), b = a;
            hook (b, [] (auto& h) { h.lookupAtStart = true; });
            auto La = collectInto (a), Lb = collectInto (b);
            renderAll ({ a, b });
            auto at35 = [] (const eg22::RecLog& L) -> const Rec* { for (const auto& r : L.r) if (r.onsetSample == 168000) return &r; return nullptr; };
            const auto* ra = at35 (*La);
            const auto* rb = at35 (*Lb);
            const double startS = ra ? (ra->anchor) / fs : -1.0;
            row (ra && rb && ra->followHit && ct (ra->f0, B) <= 10.0 && rb->followHit && ct (rb->f0, A) <= 10.0,
                 "FR9 800 ms grain, content 300 ms before the A -> B boundary: midpoint reads B; FR9n lookupAtStart reads A",
                 juce::String::formatted ("start %.3f s; midpoint f0 %.2f (B %.2f), start f0 %.2f (A %.2f)", startS, ra ? (double) ra->f0 : -1.0, B,
                                          rb ? (double) rb->f0 : -1.0, A));
        }
        // FR10 — two-pass draws: follow on vs off, every randomisation on.
        {
            P rnd { { "s1_spray", 40.0f }, { "s1_size_rand", 50.0f }, { "s1_reverse_prob", 50.0f }, { "s1_stereo_spread", 80.0f },
                    { "s1_gain_rand", 60.0f }, { "s1_filter_type", 1.0f }, { "s1_filter_rand", 70.0f }, { "s1_crush", 30.0f },
                    { "s1_crush_prob", 50.0f }, { "s1_ring_mix", 40.0f }, { "s1_ring_rand", 50.0f }, { "s1_drive", 30.0f },
                    { "s1_pitch_rand", 3.0f }, { "s1_density", 60.0f } };
            Job on = followJob (6.0, eg::sineStim (227.0, fs, 0.25), 20.0f, rnd), off = on;
            off.params = off.params + P { { "pitch_follow_s1", 0.0f } };
            auto Lon = collectInto (on), Loff = collectInto (off);
            renderAll ({ on, off });
            int bad = 0, n = 0, hits = 0, pitchDiff = 0;
            auto eq = [] (const void* x, const void* y, size_t k) { return std::memcmp (x, y, k) == 0; };
            for (size_t k = 0; k < Lon->r.size() && k < Loff->r.size(); ++k)
            {
                const auto& a = Lon->r[k], &b = Loff->r[k];
                ++n;
                hits += a.followHit ? 1 : 0;
                pitchDiff += a.rate != b.rate ? 1 : 0;
                const bool same = a.globalIndex == b.globalIndex && a.dir == b.dir && eq (&a.gainL, &b.gainL, 4) && eq (&a.gainR, &b.gainR, 4)
                               && a.windowStep == b.windowStep && eq (&a.tiltT, &b.tiltT, 4) && eq (&a.len, &b.len, 8) && a.fxActive == b.fxActive
                               && a.svfMode == b.svfMode && eq (&a.fcHz, &b.fcHz, 4) && eq (&a.ringHz, &b.ringHz, 4) && a.crushed == b.crushed
                               && eq (&a.driveD, &b.driveD, 4) && a.onsetSample == b.onsetSample;
                bad += same ? 0 : 1;
            }
            row (bad == 0 && n > 100 && hits > 100 && pitchDiff > 0 && Lon->r.size() == Loff->r.size(),
                 "FR10 follow on vs off, same seed: dir, gains, window, tilt, len, FX latch bitwise per globalIndex (only pitch / rate / level / set / delay move)",
                 juce::String::formatted ("%d grains, %d hits, %d rates moved, %d mismatches", n, hits, pitchDiff, bad));
        }
    }

    void absRows()
    {
        std::printf ("\n-- AQ. absolute quantiser (D19; R70-R72; R71 nearest)\n");
        const double fs = 48000.0;
        // AQ1 — D19: scale Off → chromatic from the root.
        {
            std::vector<Job> jobs;
            std::vector<std::shared_ptr<eg22::RecLog>> logs;
            struct C { int root; float prand, pitch; };
            std::vector<C> cs;
            for (int root : { 0, 7, 11 })
                for (float pr : { 0.0f, 3.0f })
                    for (float pt : { 0.0f, 7.0f, -5.0f })
                    {
                        cs.push_back ({ root, pr, pt });
                        Job j = followJob (3.5, eg::sineStim (227.0, fs, 0.25), 0.0f,
                                           P { { "scale_root", (float) root }, { "s1_pitch_rand", pr }, { "s1_pitch", pt }, { "s1_density", 60.0f } });
                        logs.push_back (collectInto (j));
                        jobs.push_back (j);
                    }
            // follow off with scale Off vs every follow param off (and root moved): records identical
            Job f1 = followJob (2.0, eg::sineStim (227.0, fs, 0.25), 0.0f, P { { "pitch_follow_s1", 0.0f }, { "scale_root", 5.0f } });
            Job f0 = followJob (2.0, eg::sineStim (227.0, fs, 0.25), 0.0f, P { { "pitch_follow_s1", 0.0f } });
            logs.push_back (collectInto (f1)); jobs.push_back (f1);
            logs.push_back (collectInto (f0)); jobs.push_back (f0);
            auto o = renderAll (jobs);
            Tally t;
            double worst = 0.0;
            for (size_t k = 0; k < cs.size(); ++k)
            {
                int hits = 0, bad = 0;
                for (const auto& r : logs[k]->r)
                {
                    if (! r.followHit) continue;
                    ++hits;
                    const double rel = r.target - 60.0 - cs[k].root;
                    const double off = std::abs (rel - std::round (rel)) * 100.0;
                    worst = juce::jmax (worst, off);
                    if (off > 0.001 || std::abs (r.target - r.f0) > 24.0 + 1e-9) ++bad;
                }
                t.check (hits >= 100 && bad == 0, juce::String::formatted ("root %d prand %.0f pitch %+.0f: %d hits, %d bad", cs[k].root, cs[k].prand, cs[k].pitch, hits, bad));
            }
            const auto& La = *logs[cs.size()], &Lb = *logs[cs.size() + 1];
            bool same = La.r.size() == Lb.r.size() && ! La.r.empty();
            for (size_t k = 0; same && k < La.r.size(); ++k) same = eg25x::samePlan (La.r[k], Lb.r[k]);
            row (t.bad == 0, "AQ1 D19 follow on, scale Off: every target within 0.001 c of 60 + root + integer; |interval| <= 24 st; >= 100 hits per case",
                 t.detail ((int) cs.size()) + juce::String::formatted ("  worst %.2e c", worst));
            row (same && o[cs.size()].eng().diag.followLookups == 0, "AQ1 follow off with scale Off: 0 lookups, records == every follow param off",
                 juce::String::formatted ("%d records", (int) La.r.size()));
        }
        // AQ2 — scales with masks at root 3.
        {
            struct C { const char* name; int choice; std::uint64_t mask; const char* custom; };
            const int major = scaleChoice ("Major"), e19 = scaleChoice ("19-EDO"), bp = scaleChoice ("Bohlen-Pierce");
            std::vector<C> cs { { "Major {0,2,4}", major, 0x15ull, nullptr }, { "19-EDO even", e19, 0x55555ull, nullptr },
                                { "BP {0,3,6,9}", bp, (1ull << 0) | (1ull << 3) | (1ull << 6) | (1ull << 9), nullptr },
                                { "Custom edo 9 1800 {0,2,4,5,7}", 31, (1ull << 0) | (1ull << 2) | (1ull << 4) | (1ull << 5) | (1ull << 7), "edo 9 1800" } };
            std::vector<Job> jobs;
            std::vector<std::shared_ptr<eg22::RecLog>> logs;
            for (const auto& c : cs)
                for (float pr : { 0.0f, 5.0f })
                {
                    Job j = followJob (pr > 0.0f ? 12.0 : 3.0, eg::sineStim (227.0, fs, 0.25), 0.0f,
                                       P { { "scale_root", 3.0f }, { "s1_scale", (float) c.choice }, { "s1_pitch_rand", pr }, { "s1_density", pr > 0.0f ? 200.0f : 60.0f } });
                    const auto mask = c.mask; const int ch = c.choice; const char* cu = c.custom;
                    auto prev = j.pre;
                    j.pre = [prev, mask, ch, cu] (Proc& p)
                    {
                        if (prev) prev (p);
                        juce::String bad;
                        if (cu != nullptr) p.setCustomScale ("", cu, bad);
                        p.setDegreeMask (1, ch, mask);
                    };
                    logs.push_back (collectInto (j));
                    jobs.push_back (j);
                }
            auto o = renderAll (jobs);
            Tally tm, tn, tr;
            for (size_t ci = 0; ci < cs.size(); ++ci)
            {
                // the member set A from the table (independent of the engine's AbsSet)
                auto& proc = *o[ci * 2].p;
                const auto* tab = proc.getCollectionTable (cs[ci].choice);
                const double P0 = (double) tab->period, R = 6000.0 + 300.0;
                std::vector<double> mem;
                for (int kk = -8; kk <= 8; ++kk)
                    for (int d = 0; d < tab->count; ++d)
                        if ((cs[ci].mask >> d) & 1u) mem.push_back (R + (double) tab->cents[(size_t) d] + kk * P0);
                std::sort (mem.begin(), mem.end());
                auto inA = [&mem] (double c) { for (double m : mem) if (std::abs (m - c) <= 1e-6) return true; return false; };
                for (int v = 0; v < 2; ++v)
                {
                    const auto& L = *logs[ci * 2 + (size_t) v];
                    int hits = 0, notMember = 0, notNearest = 0, outWin = 0;
                    std::map<long long, int> counts;
                    for (const auto& r : L.r)
                    {
                        if (! r.followHit) continue;
                        ++hits;
                        const double tc = 100.0 * r.target, T = 100.0 * (r.f0 + 0.0);
                        if (! inA (tc)) ++notMember;
                        if (v == 0)
                        {
                            double best = 1e300; double bm = 0.0;
                            for (double m : mem)
                            {
                                if (std::abs (m - 100.0 * r.f0) > 2400.0) continue;
                                const double d = std::abs (m - T);
                                if (d < best - 1e-9 || (std::abs (d - best) <= 1e-9 && m > bm)) { best = d; bm = m; }
                            }
                            if (std::abs (bm - tc) > 1e-6) ++notNearest;
                        }
                        else
                        {
                            if (tc < T - 500.0 - 1e-6 || tc > T + 500.0 + 1e-6 || std::abs (tc - 100.0 * r.f0) > 2400.0 + 1e-6) ++outWin;
                            counts[std::llround (tc * 1000.0)]++;
                        }
                    }
                    if (v == 0)
                    {
                        tm.check (hits > 50 && notMember == 0, juce::String (cs[ci].name) + juce::String::formatted (" prand 0: %d hits, %d not in A", hits, notMember));
                        tn.check (notNearest == 0, juce::String (cs[ci].name) + juce::String::formatted (": %d not nearest", notNearest));
                    }
                    else
                    {
                        // χ² over the window members (uniform pick, R72)
                        double chi = 0.0;
                        const double expct = (double) hits / juce::jmax (1, (int) counts.size());
                        for (auto& kv : counts) chi += (kv.second - expct) * (kv.second - expct) / expct;
                        const int dof = (int) counts.size() - 1;
                        // p > 0.001 ⇔ chi < the 0.999 quantile; Wilson–Hilferty approximation
                        const double z = 3.090232;
                        const double q999 = dof > 0 ? dof * std::pow (1.0 - 2.0 / (9.0 * dof) + z * std::sqrt (2.0 / (9.0 * dof)), 3.0) : 0.0;
                        tm.check (hits >= 2000 && notMember == 0, juce::String (cs[ci].name) + juce::String::formatted (" prand 5: %d hits, %d not in A", hits, notMember));
                        tr.check (outWin == 0 && dof >= 1 && chi < q999, juce::String (cs[ci].name)
                                      + juce::String::formatted (": %d outside [T +- span] ^ clamp; chi2 %.1f (dof %d, crit %.1f)", outWin, chi, dof, q999));
                    }
                }
            }
            row (tm.bad == 0, "AQ2 Major / 19-EDO / BP / Custom (edo 9 1800) masked, root 3: every target in A = {6300 + c_d + k P} (1e-6 c)", tm.detail (8));
            row (tn.bad == 0, "AQ2 pitch_rand 0: the nearest member to T (ties up), recomputed from the table", tn.detail (4));
            row (tr.bad == 0, "AQ2 pitch_rand 5 st: every target in [T +- span] ^ clamp; pick distribution chi2 p > 0.001 (>= 2000 grains)", tr.detail (4));
        }
        // AQ3 / AQ4 / AQ5 / AQ6 — AbsSet units.
        {
            const auto& bi = PitchCollections::builtIns();
            const auto& chrom = bi[(size_t) PitchCollections::Chromatic];
            const auto& majorT = bi[(size_t) PitchCollections::Major];
            const auto& bpT = bi[(size_t) PitchCollections::BohlenPierce];
            ouaricon::grain::CounterRng::Draw draw (0x1234ull, ogran::kGrainStream, 77ull);
            Tally t3;
            struct S { const char* name; const PitchCollections::Table* t; std::uint64_t mask; bool all; };
            const S sets[] { { "Chromatic", &chrom, ~0ull, true }, { "Major masked", &majorT, 0x15ull, false }, { "BP", &bpT, ~0ull, false } };
            for (const auto& st : sets)
            {
                auto a = std::make_unique<ogran::AbsSet>();
                a->rebuildIfNeeded (1, st.mask, 0, *st.t, st.all);
                for (double f0 : { 28.0, 100.0 })
                    for (double pitch : { -24.0, 24.0 })
                        for (double pr : { 0.0, 24.0 })
                            for (int rep = 0; rep < 20; ++rep)
                            {
                                ouaricon::grain::CounterRng::Draw d2 (0x99ull + (std::uint64_t) rep, ogran::kGrainStream, (std::uint64_t) rep);
                                double tc = 0.0;
                                const int res = a->target (f0, pitch, pr, 4, d2, tc);
                                const double R = 6000.0 + 400.0;
                                bool member = false;
                                for (int q = 0; q < a->size(); ++q)
                                    for (int k0 = -12; k0 <= 12 && ! member; ++k0)
                                        member = std::abs (tc - (R + k0 * a->getPeriod() + a->data()[q])) <= 1e-6;
                                t3.check (res == 0 && std::abs (tc / 100.0 - f0) <= 24.0 + 1e-9 && member,
                                          juce::String (st.name) + juce::String::formatted (" f0 %.0f pitch %+.0f pr %.0f: res %d interval %.3f", f0, pitch, pr, res, tc / 100.0 - f0));
                            }
            }
            row (t3.bad == 0, "AQ3 AbsSet unit: f0 {28, 100} x pitch +-24 x pitch_rand {0, 24} x {Chromatic, Major masked, BP}: |interval| <= 24 st, member in A",
                 t3.detail (3 * 2 * 2 * 2 * 20));

            // AQ4 nearest, ties up.
            auto ch = std::make_unique<ogran::AbsSet>();
            ch->rebuildIfNeeded (0, ~0ull, 0, chrom, true);
            double tc = 0.0;
            ch->target (60.5, 0.0, 0.0, 0, draw, tc);            // exactly between 60 and 61 → 61
            const double tie = tc;
            ch->target (61.0, 0.0, 0.0, 0, draw, tc);            // on a member
            const double on = tc;
            ch->target (60.49, 0.0, 0.0, 0, draw, tc);
            const double below = tc;
            row (std::abs (tie - 6100.0) < 1e-9 && std::abs (on - 6100.0) < 1e-9 && std::abs (below - 6000.0) < 1e-9,
                 "AQ4 nearest (R71): T exactly between two members -> the upper; on a member -> that member",
                 juce::String::formatted ("tie %.3f, on %.3f, 60.49 -> %.3f", tie, on, below));

            // AQ5 fallbacks: hand-built 6000 c period, one degree → NoMember; an empty mask → EmptySet.
            PitchCollections::Table wide;
            wide.count = 1; wide.cents[0] = 0.0f; wide.period = 6000.0f;
            auto w = std::make_unique<ogran::AbsSet>();
            w->rebuildIfNeeded (2, 1ull, 0, wide, false);
            const int nm = w->target (90.0, 0.0, 0.0, 0, draw, tc);   // T − R = 3000 → members 0 / 6000 both > 24 st away
            auto e = std::make_unique<ogran::AbsSet>();
            e->rebuildIfNeeded (3, 0ull, 0, majorT, false);
            const int em = e->target (60.0, 0.0, 0.0, 0, draw, tc);
            row (nm == ogran::AbsSet::NoMember && em == ogran::AbsSet::EmptySet, "AQ5 AbsSet unit: one degree with a 6000 c period -> NoMember (4); empty mask -> EmptySet (3)",
                 juce::String::formatted ("%d / %d", nm, em));

            // AQ6 register (R70): BP at roots 0 and 5.
            auto b = std::make_unique<ogran::AbsSet>();
            b->rebuildIfNeeded (4, ~0ull, 0, bpT, false);
            int badSet = 0, n = 0;
            for (double f0 = 40.0; f0 < 90.0; f0 += 0.37)
            {
                double t0 = 0.0, t5 = 0.0;
                b->target (f0, 0.0, 0.0, 0, draw, t0);
                b->target (f0 + 5.0, 0.0, 0.0, 5, draw, t5);
                ++n;
                if (std::abs (t5 - t0 - 500.0) > 1e-6) ++badSet;
                bool member = false;
                for (int k0 = -5; k0 <= 5 && ! member; ++k0)
                    for (int d = 0; d < bpT.count && ! member; ++d)
                        member = std::abs (t0 - (6000.0 + (double) bpT.cents[(size_t) d] + k0 * (double) bpT.period)) <= 1e-6;
                if (! member) ++badSet;
            }
            row (badSet == 0, "AQ6 register (R70): BP at roots 0 / 5 differ by exactly +500 c; members == {6000 + 100 root + c_d + k 1901.955}",
                 juce::String::formatted ("%d/%d", n - badSet, n));
        }
        // AQ3 engine row, AQ4 follow-off, AQ5 engine fallbacks, AQ7 rebuild keys, AQ8 sounding pitch.
        {
            const int major = scaleChoice ("Major");
            std::vector<Job> jobs;
            std::vector<std::shared_ptr<eg22::RecLog>> logs;
            auto add = [&] (Job j) { logs.push_back (collectInto (j)); jobs.push_back (j); };
            add (followJob (3.0, eg::sineStim (41.2, fs, 0.25), 0.0f, P { { "s1_pitch", -24.0f }, { "s1_pitch_rand", 24.0f }, { "s1_density", 60.0f } }));   // 0 AQ3
            add (followJob (2.0, eg::sineStim (227.0, fs, 0.25), 0.0f, P { { "pitch_follow_s1", 0.0f } }));                                                 // 1 AQ4 follow off
            { Job j = followJob (2.0, eg::sineStim (227.0, fs, 0.25), 0.0f, P { { "s1_scale", (float) major }, { "s1_pitch", 3.0f }, { "s1_pitch_rand", 5.0f } });
              auto prev = j.pre; j.pre = [prev, major] (Proc& p) { if (prev) prev (p); p.setDegreeMask (1, major, 0ull); }; add (j); }                       // 2 AQ5 empty mask
            add (followJob (2.0, [] (juce::int64 i, int) { return (float) (0.1 * gauss (i, 81)); }, 0.0f, P { { "s1_pitch", 3.0f } }));                     // 3 AQ5 unconfident
            {
                Job j = followJob (4.0, eg::sineStim (227.0, fs, 0.25), 0.0f,
                                   P { { "s1_scale", (float) major }, { "s2_enable", 1.0f }, { "pitch_follow_s2", 1.0f }, { "s2_scale", 31.0f },
                                       { "lfo1_rate", 3.0f } } + route (1, 1, ogran::ModDest::stream (0, ogran::ModDest::PitchRand), 50.0f));
                j.events.push_back ({ (juce::int64) (1.0 * fs), [major] (Proc& p) { p.setDegreeMask (1, major, 0x95ull); } });
                j.events.push_back ({ (juce::int64) (2.0 * fs), [] (Proc& p) { juce::String b; p.setCustomScale ("", "edo 7", b); } });
                add (j);                                                                                                                                  // 4 AQ7
            }
            add (followJob (3.0, eg::sineStim (227.0, fs, 0.25), 0.0f, P { { "s1_size", 400.0f }, { "s1_density", 2.0f } }));                         // 5 AQ8 (isolated grains)
            auto o = renderAll (jobs);
            // AQ3 engine
            int iv = 0, hits3 = 0;
            double minIv = 1e9;
            for (const auto& r : logs[0]->r) if (r.followHit) { ++hits3; minIv = juce::jmin (minIv, r.target - r.f0); if (r.target - r.f0 < -24.0 - 1e-9) ++iv; }
            row (hits3 > 50 && iv == 0, "AQ3 engine: f0 41.2 Hz (MIDI 28), pitch -24, pitch_rand 24: every recorded interval >= -24 st",
                 juce::String::formatted ("%d hits, min interval %.3f st", hits3, minIv));
            // AQ4 follow off: offset 0 (interval mode)
            int off0 = 0, n4 = 0;
            for (const auto& r : logs[1]->r) { ++n4; off0 += (r.pitchCents == 0.0f && r.rate == 1.0 && ! r.followHit && r.followMiss == 0) ? 1 : 0; }
            row (n4 > 10 && off0 == n4 && o[1].eng().diag.followLookups == 0, "AQ4 follow off, same settings: offset 0, rate 1 (interval mode unchanged)",
                 juce::String::formatted ("%d/%d", off0, n4));
            // AQ5 engine
            int m3 = 0, n5 = 0, m1 = 0, n5u = 0;
            for (const auto& r : logs[2]->r) if ((double) r.onsetSample > 0.2 * fs) { ++n5; m3 += (r.followMiss == 3 && r.pitchCents == 0.0f && r.rate == std::exp2 (3.0 / 12.0)) ? 1 : 0; }
            for (const auto& r : logs[3]->r) if ((double) r.onsetSample > 0.2 * fs) { ++n5u; m1 += (r.followMiss == 1 && r.rate == std::exp2 (3.0 / 12.0)) ? 1 : 0; }
            row (n5 > 10 && m3 == n5 && n5u > 10 && m1 == n5u, "AQ5 engine: empty mask -> followMiss 3, st == pitch exactly, no pick; unconfident (noise) -> 1; both interval mode",
                 juce::String::formatted ("empty %d/%d; unconfident %d/%d", m3, n5, m1, n5u));
            // AQ7 rebuild keys: 2 initial (s1 Major, s2 Custom) + 1 mask change + 1 custom apply
            row (o[4].eng().diag.absSetRebuilds == 4, "AQ7 abs-set rebuilds == key changes (pitch_rand LFO-modulated: 0 extra); a mask change and a custom apply add 1 each",
                 juce::String::formatted ("%lld rebuilds (expected 4)", (long long) o[4].eng().diag.absSetRebuilds));
            // AQ8 (INFO) sounding pitch: the output through the MPM reference.
            {
                const auto& L = o[5].L;
                ogran::Decimator dc;
                dc.prepare (4);
                std::vector<float> dec;
                for (size_t i = 0; i < L.size(); ++i) { dc.push (L[i]); if (i % 4 == 0) dec.push_back (dc.output()); }
                double worst = 0.0;
                int nWin = 0;
                double target = 0.0;
                for (const auto& r : logs[5]->r) if (r.followHit) target = r.target;
                // One window (85 ms) centred on each isolated grain's middle (grains at k·0.5 s,
                // 400 ms Hann, no overlap): a window spanning a grain-to-grain crossfade sees
                // the phase reset between grains and reads a blend.
                for (int k = 1; k < 6; ++k)
                {
                    const size_t st = (size_t) ((0.5 * k + 0.2) * fs / 4.0) - 512;
                    if (st + 1024 > dec.size()) break;
                    const auto rm = eg25x::refMpm (dec.data() + st, 12000.0);
                    if (! rm.confident) continue;
                    const double note = 69.0 + 12.0 * std::log2 ((12000.0 / rm.tau) / 440.0);
                    worst = juce::jmax (worst, std::abs (note - target) * 100.0);
                    ++nWin;
                }
                eg25x::info (juce::String::formatted ("AQ8 sounding pitch of isolated follow grains on 227 Hz (target %.3f): worst %.2f c over %d grains (R72: within the MPM error)",
                                                      target, worst, nWin));
            }
        }
    }

    //==========================================================================
    // MX1–MX2a — FUNC-06: Pitch / Onset × class, removal; QB1–QB2 — QUAL-01 bursts (T9)
    //==========================================================================
    namespace D = ogran::ModDest;

    /** A stepped off-grid tone (233 → 311 → 227 Hz) + 2 ms 1 kHz Hann bursts every 250 ms. */
    inline Stim toneBurstStim (double fs, double lenS, double ioi = 0.25)
    {
        auto x = steppedSine (fs, lenS, { { 0.0, 233.0 }, { lenS / 3.0, 311.0 }, { 2.0 * lenS / 3.0, 227.0 } }, 0.2);
        for (double t = 0.05; t < lenS - 0.01; t += ioi) addBurst (*x, fs, t, 2.0, 1000.0, 0.5);
        return fromBuffer (x);
    }

    void matrixRows()
    {
        std::printf ("\n-- MX. FUNC-06: Pitch / Onset x class (bipolar), removal without a step\n");
        const double fs = 48000.0;
        // MX1 — {Pitch, Onset} × 5 classes × ±100 %.
        {
            struct Cls { const char* name; int dest; float base; double baseEng, lo, hi; bool latched; };
            const Cls classes[] {
                { "mix (ramped)", D::Mix, 50.0f, 0.5, 0.0, 1.0, false },
                { "s1_spray (latched)", D::stream (0, D::Spray), 30.0f, 0.3, 0.0, 1.0, true },
                { "s1_size (skewed)", D::stream (0, D::Size), 100.0f, 100.0, 5.0, 2000.0, true },
                { "reverb_size", D::RevSize, 50.0f, 0.5, 0.0, 1.0, false },
                { "lfo1 rate", D::LfoRate1, 1.0f, 1.0, 0.01, 20.0, false } };
            std::vector<Job> jobs;
            std::vector<std::shared_ptr<eg25b::Cells>> cells;
            struct Meta { int src; Cls cls; float depth; };
            std::vector<Meta> meta;
            for (int src : { ogran::ModSrc::Pitch, ogran::ModSrc::Onset })
                for (const auto& c : classes)
                    for (float depth : { 100.0f, -100.0f })
                    {
                        Job j = eg25b::modJob (48000, P { { "lfo1_rate", 1.0f }, { GranulatorParamIds::kModDestIds[c.dest], c.base } } + route (1, src, c.dest, depth), c.dest);
                        j.stim = toneBurstStim (fs, 1.0);
                        cells.push_back (eg25b::recordSources (j));
                        jobs.push_back (j);
                        meta.push_back ({ src, c, depth });
                    }
            auto outs = renderAll (jobs);
            Tally t;
            for (size_t k = 0; k < jobs.size(); ++k)
            {
                const auto& m = meta[k];
                const auto tl = eg25b::traceList (outs[k].eng());
                const auto& sv = cells[k]->src[m.src];
                int wrong = 0, out = 0, checked = 0;
                for (const auto& [at, v] : tl)
                {
                    const size_t cell = (size_t) (at / 32);
                    if (cell < 1 || cell >= sv.size()) continue;
                    if (v < m.cls.lo - 1e-6 || v > m.cls.hi + 1e-6 || ! std::isfinite (v)) ++out;
                    const float a = sv[cell], b = m.cls.latched ? sv[cell - 1] : sv[cell];
                    if (std::abs (a) < 0.02f || std::abs (b) < 0.02f) continue;
                    const double want = m.depth > 0.0f ? 1.0 : -1.0;   // both sources are unipolar (>= 0)
                    if (! ((v - m.cls.baseEng) * want > 0.0)) ++wrong;
                    ++checked;
                }
                t.check (wrong == 0 && out == 0 && checked >= 5, juce::String (m.src == ogran::ModSrc::Pitch ? "Pitch -> " : "Onset -> ") + m.cls.name
                                                                     + (m.depth > 0 ? " +" : " -") + juce::String::formatted (": %d wrong, %d out, %d checked", wrong, out, checked));
            }
            row (t.bad == 0, "MX1 Pitch / Onset x {mix, s1_spray, s1_size, reverb_size, lfo rate} x +-100 %: moves with sign (depth.src), stays in range",
                 t.detail ((int) jobs.size()));
        }
        // MX2 / MX2n / MX2a — removal (source → None, dest → None, depth → 0, dest swap).
        {
            const juce::int64 tR = 938 * 32;
            const char* kinds[] { "source -> None", "dest -> None", "depth -> 0", "dest swap" };
            std::vector<Job> mj;
            std::vector<std::shared_ptr<eg25b::Cells>> cells;
            struct MM { int src, kind; bool noRes; };
            std::vector<MM> mm;
            for (int sid : { ogran::ModSrc::Pitch, ogran::ModSrc::Onset })
                for (int kind = 0; kind < 4; ++kind)
                    for (bool noRes : { false, true })
                    {
                        if (noRes && kind != 0) continue;
                        Job j = eg25b::modJob (2 * 24000, P { { "mix", 50.0f } } + route (1, sid, D::Mix, 60.0f), D::Mix);
                        j.stim = toneBurstStim (fs, 1.0);
                        j.events.push_back ({ tR, [kind] (Proc& p)
                        {
                            if (kind == 0) eg::setP (p, "mod1_source", 0.0f);
                            if (kind == 1) eg::setP (p, "mod1_dest", 0.0f);
                            if (kind == 2) eg::setP (p, "mod1_depth", 0.0f);
                            if (kind == 3) eg::setP (p, "mod1_dest", (float) D::stream (0, D::Pan));
                        } });
                        if (noRes) hook (j, [] (auto& h) { h.noResidual = true; });
                        cells.push_back (eg25b::recordSources (j));
                        mj.push_back (j);
                        mm.push_back ({ sid, kind, noRes });
                    }
            auto mouts = renderAll (mj);
            Tally t2;
            double nWorst = 0.0, nBound = 0.0;
            for (size_t k = 0; k < mj.size(); ++k)
            {
                const auto tm = eg25b::traceMap (mouts[k].eng());
                const auto& sv = cells[k]->src[mm[k].src];
                const double slope = mm[k].src == ogran::ModSrc::Pitch ? (double) eg25x::an (mouts[k]).getTracker().slewCoeff() / 32.0 : 1.0 / (0.15 * fs);
                const double c = 0.6 * (double) sv[(size_t) (tR / 32 - 1)];
                const double bound = 1.05 * (0.6 * slope + std::abs (c) / (30.0 * 32.0));
                double worst = 0.0;
                bool rawAfter = true;
                for (juce::int64 cc = tR / 32 - 200; cc < tR / 32 + 200; ++cc)
                {
                    const bool attack = mm[k].src == ogran::ModSrc::Onset && sv[(size_t) cc] > sv[(size_t) cc - 1];   // the instant attack (excluded by design)
                    const double a = tm.count ((cc - 1) * 32) ? tm.at ((cc - 1) * 32) : 0.0, b = tm.count (cc * 32) ? tm.at (cc * 32) : 0.0;
                    if (! attack || cc >= tR / 32) worst = juce::jmax (worst, std::abs (b - a) / 32.0);
                    if (cc >= tR / 32 + 31) rawAfter = rawAfter && juce::exactlyEqual ((float) b, 0.5f);
                }
                if (mm[k].noRes) { if (mm[k].src == ogran::ModSrc::Pitch) { nWorst = worst; nBound = bound; } continue; }
                t2.check (worst <= bound && rawAfter && ! mouts[k].eng().getMatrix().routed (D::Mix),
                          juce::String (mm[k].src == ogran::ModSrc::Pitch ? "Pitch " : "Onset ") + kinds[mm[k].kind]
                              + juce::String::formatted (": max |dv|/sample %.3g (bound %.3g)%s", worst, bound, rawAfter ? "" : ", not raw after 20 ms"));
            }
            row (t2.bad == 0, "MX2 Pitch / Onset route removal x 4 kinds: |dv| per sample <= the P56 bound through the 20 ms tail, then exactly the raw path",
                 t2.detail (8));
            row (nWorst > 2.0 * nBound, "MX2n noResidual: Pitch source -> None steps past the bound", juce::String::formatted ("%.3g (bound %.3g)", nWorst, nBound));

            Job held = eg25b::modJob (2 * 24000, P { { "mix", 50.0f } } + route (1, ogran::ModSrc::Pitch, D::Mix, 60.0f), -1);
            auto carrier = steppedSine (fs, 1.0, { { 0.0, 440.0 } }, 0.4);
            for (double tt = 0.05; tt < 0.99; tt += 0.25) addBurst (*carrier, fs, tt, 2.0, 1000.0, 0.3);
            held.stim = fromBuffer (carrier);
            Job rem = held;
            rem.events.push_back ({ tR, [] (Proc& p) { eg::setP (p, "mod1_source", 0.0f); } });
            auto ko = renderAll ({ held, rem });
            const double t0 = (double) tR / fs - 0.03, t1 = (double) tR / fs + 0.05;
            const double kh = eg24b::kappa (ko[0], t0, t1), kr = eg24b::kappa (ko[1], t0, t1);
            row (kr <= 2.0 * kh, "MX2a audio: Pitch -> mix removal step-click kappa <= 2.0x the held-route reference (440 Hz + bursts)",
                 juce::String::formatted ("%.2f vs %.2f (%.2fx)", kr, kh, kr / kh));
        }
    }

    /** The P75 swell carrier: 440 Hz at 0.03 with +18 dB swells (1 ms raised-cosine
        attack, 20 ms hold, 30 ms decay) every 125 ms. */
    inline std::shared_ptr<std::vector<float>> swellCarrier (double fs, double lenS)
    {
        auto x = std::make_shared<std::vector<float>> ((size_t) (lenS * fs));
        const double up = std::pow (10.0, 18.0 / 20.0);
        for (size_t i = 0; i < x->size(); ++i)
        {
            const double t = (double) i / fs, tc = t - 0.125 * std::floor (t / 0.125);
            double e = 1.0;
            if (tc < 0.001) e = 1.0 + (up - 1.0) * 0.5 * (1.0 - std::cos (kPi * tc / 0.001));
            else if (tc < 0.021) e = up;
            else if (tc < 0.051) e = 1.0 + (up - 1.0) * 0.5 * (1.0 + std::cos (kPi * (tc - 0.021) / 0.030));
            (*x)[i] = (float) (0.03 * e * std::sin (2.0 * kPi * 440.0 * t));
        }
        return x;
    }

    void burstClickRows()
    {
        std::printf ("\n-- QB. QUAL-01 bursts: step-click kappa vs a same-grain-count reference (R78)\n");
        const double fs = 48000.0, lenS = 6.0;
        auto car = swellCarrier (fs, lenS);
        auto trigJob = [&] (bool tempo, float window)
        {
            Job j = anaJob (fs, lenS, fromBuffer (car), eg22::base ({ 1 }) + P { { "s1_onset_trigger", 1.0f }, { "s1_window", window }, { "s1_size", 60.0f },
                                                                              { "grain_limit", 3.0f }, { "s1_jitter", 0.0f } });
            if (tempo)
            {
                j.params = j.params + P { { "sync_mode", 1.0f }, { "s1_density_sync", 7.0f }, { "s1_size_sync", 2.0f } };
                eg25::withTransport (j, Transport (fs, 120.0));
            }
            else j.params = j.params + P { { "s1_density", 20.0f } };
            return j;
        };
        std::vector<Job> trig { trigJob (false, 0.0f), trigJob (true, 0.0f), trigJob (false, 1.0f), trigJob (true, 1.0f) };
        auto ot = renderAll (trig);
        auto rate = [&] (const Out& o) { return (double) o.eng().diag.spawns / lenS; };
        std::vector<Job> refs;
        for (int k = 0; k < 2; ++k)
        {
            Job r = anaJob (fs, lenS, fromBuffer (car), eg22::base ({ 1 }) + P { { "s1_window", 0.0f }, { "s1_size", 60.0f }, { "grain_limit", 3.0f },
                                                                              { "s1_jitter", 0.0f }, { "s1_density", (float) rate (ot[(size_t) k]) } });
            refs.push_back (r);
        }
        auto orf = renderAll (refs);
        const double t0 = 0.5, t1 = lenS - 0.1;
        juce::String d;
        bool ok = true, nOk = true;
        const char* nm[] { "Free", "Tempo" };
        for (int k = 0; k < 2; ++k)
        {
            const double kt = eg24b::kappa (ot[(size_t) k], t0, t1), kr = eg24b::kappa (orf[(size_t) k], t0, t1), kn = eg24b::kappa (ot[(size_t) k + 2], t0, t1);
            ok = ok && kt <= 2.0 * kr && ot[(size_t) k].eng().diag.onsetEvents > 20;
            nOk = nOk && kn > 2.0 * kr;
            d << nm[k] << juce::String::formatted (" %.2fx (%.1f g/s, %lld events), QB1n %.2fx; ", kt / kr, rate (ot[(size_t) k]),
                                                   (long long) ot[(size_t) k].eng().diag.onsetEvents, kn / kr);
        }
        row (ok, "QB1 burst click kappa <= 2.0x the same-grain-count reference (Free 20 g/s; Tempo 1/16 @ 120; Hann 60 ms; swell carrier)", d.trim());
        row (nOk, "QB1n the same trigger renders with s1_window 1 (instant attack) exceed 2.0x (the gate sees a click here)", d.trim());
        // QB2 (INFO) — Tempo burst level (R78).
        {
            auto busy = burstTrain (fs, lenS, 0.05, 0.055, 100, 2.0, 1000.0, 0.5);
            auto tj = [&] (bool on)
            {
                Job j = anaJob (fs, lenS, fromBuffer (busy), eg22::base ({ 1 }) + P { { "sync_mode", 1.0f }, { "s1_density_sync", 7.0f }, { "s1_size_sync", 4.0f },
                                                                                   { "s1_onset_trigger", on ? 1.0f : 0.0f }, { "grain_limit", 3.0f } });
                eg25::withTransport (j, Transport (fs, 120.0));
                return j;
            };
            auto o = renderAll ({ tj (true), tj (false) });
            const double dB = eg::rmsDb (o[0].L, (size_t) fs, (size_t) (lenS * fs)) - eg::rmsDb (o[1].L, (size_t) fs, (size_t) (lenS * fs));
            eg25x::info (juce::String::formatted ("QB2 Tempo 1/16 @ 120, busy burst train: trigger-on vs off RMS %+.2f dB (R78: <= +5.4 dB expected); bursts %lld",
                                                  dB, (long long) o[0].eng().diag.tempoBursts));
        }
    }

    //==========================================================================
    // N9 / M6 / Z6 / C6 — PERF-03, PERF-01, QUAL-02, PERF-02 (T10)
    //==========================================================================
    /** The N9 input: stepped off-grid tones, 2 ms bursts every 180 ms, a 300 ms
        noise segment and 200 ms of silence (12 s). */
    inline std::shared_ptr<std::vector<float>> n9Input (double fs, double lenS)
    {
        auto x = steppedSine (fs, lenS, { { 0.0, 233.0 }, { 3.0, 311.0 }, { 6.0, 227.0 }, { 9.0, 262.5 } }, 0.2);
        for (double t = 0.1; t < lenS - 0.01; t += 0.18) addBurst (*x, fs, t, 2.0, 1000.0, 0.5);
        for (size_t i = 0; i < x->size(); ++i)
        {
            const double t = (double) i / fs;
            if (t >= 4.0 && t < 4.3) (*x)[i] = (float) (0.15 * gauss ((juce::int64) i, 99));
            if (t >= 7.5 && t < 7.7) (*x)[i] = 0.0f;
        }
        return x;
    }

    /** Every 2.5b element on: triggers ×4, follow ×4 (Off, Major, 19-EDO, BP), the
        Pitch / Onset routes, feedback 80 %, freeze toggles at mid-cell samples. */
    inline Job n9Base (bool tempo, double lenS = 12.0)
    {
        const double fs = 48000.0;
        Job j = anaJob (fs, lenS, fromBuffer (n9Input (fs, lenS)), eg22::base ({ 1, 2, 3, 4 }) + P { { "feedback", 80.0f }, { "grain_limit", 3.0f } });
        const float dens[4] { 20, 15, 25, 10 }, size[4] { 120, 200, 80, 300 }, pos[4] { 0, 10, 25, 5 };
        const int sc[4] { 0, scaleChoice ("Major"), scaleChoice ("19-EDO"), scaleChoice ("Bohlen-Pierce") };
        for (int s = 1; s <= 4; ++s)
            j.params = j.params + P { { eg22::sp (s, "onset_trigger"), 1.0f }, { "pitch_follow_s" + juce::String (s), 1.0f },
                                      { eg22::sp (s, "scale"), (float) sc[s - 1] }, { eg22::sp (s, "density"), dens[s - 1] },
                                      { eg22::sp (s, "size"), size[s - 1] }, { eg22::sp (s, "spray"), 20.0f }, { eg22::sp (s, "pitch_rand"), 3.0f },
                                      { eg22::sp (s, "jitter"), 20.0f }, { eg22::sp (s, "position"), pos[s - 1] },
                                      { eg22::sp (s, "density_sync"), (float) (6 + s) } };
        j.params = j.params + route (1, ogran::ModSrc::Pitch, D::stream (0, D::Level), 40.0f) + route (2, ogran::ModSrc::Onset, D::Mix, -30.0f)
                 + route (3, ogran::ModSrc::Pitch, D::stream (1, D::Pitch), 30.0f) + route (4, ogran::ModSrc::Onset, D::stream (2, D::Density), 40.0f)
                 + route (5, 1, D::stream (3, D::Position), 50.0f) + route (6, ogran::ModSrc::Env, D::FbSat, 40.0f) + P { { "lfo1_rate", 0.7f } };
        auto ev = [&j] (juce::int64 at, std::function<void (Proc&)> f) { j.events.push_back ({ at, std::move (f) }); };
        ev (64 * 1500 + 7,  [] (Proc& p) { eg::setP (p, "freeze", 1.0f); });
        ev (64 * 2100 + 13, [] (Proc& p) { eg::setP (p, "freeze", 0.0f); });
        ev (64 * 4000 + 5,  [] (Proc& p) { eg::setP (p, "freeze", 1.0f); });
        ev (64 * 4400 + 21, [] (Proc& p) { eg::setP (p, "freeze", 0.0f); });
        if (tempo)
        {
            j.params = j.params + P { { "sync_mode", 1.0f } };
            Transport tr (fs, 120.0);
            tr.tempo (48017, 97.3).loopWrap (192005, 4.0, 12.0).locate (360003, 21.5);
            eg25::withTransport (j, tr);
        }
        return j;
    }

    inline int vdiff (const Out& a, const Out& b)
    {
        int n = 0;
        for (size_t i = 0; i < a.L.size() && i < b.L.size(); ++i)
            if ((double) a.L[i] != (double) b.L[i] || (double) a.R[i] != (double) b.R[i]) ++n;
        return n;
    }
    inline bool sameRing (const ogran::F0Ring& a, const ogran::F0Ring& b)
    {
        if (a.size() != b.size() || a.newest() != b.newest()) return false;
        for (int k = 0; k < a.size(); ++k)
            if (a.entry (k).tag != b.entry (k).tag || std::memcmp (&a.entry (k).note, &b.entry (k).note, 4) != 0) return false;
        return true;
    }
    inline bool sameHops (const HopLog& a, const HopLog& b)
    {
        if (a.h.size() != b.h.size()) return false;
        for (size_t k = 0; k < a.h.size(); ++k)
            if (a.h[k].aHop != b.h[k].aHop || a.h[k].commitAt != b.h[k].commitAt || std::memcmp (&a.h[k].rawNote, &b.h[k].rawNote, 4) != 0
                || std::memcmp (&a.h[k].decidedNote, &b.h[k].decidedNote, 4) != 0 || a.h[k].ringResult != b.h[k].ringResult)
                return false;
        return true;
    }

    void blockNullRows()
    {
        std::printf ("\n-- N9. PERF-03 block-size null with every 2.5b element on (value compare)\n");
        for (int form = 0; form < 2; ++form)
        {
            const bool tempo = form == 1;
            const Job base = n9Base (tempo);
            std::vector<Job> jobs;
            std::vector<std::shared_ptr<EvLog>> ev;
            std::vector<std::shared_ptr<HopLog>> hp;
            for (int b : { 32, 441, 512, 4096, 0 })
            {
                Job j = base; j.block = b;
                ev.push_back (evInto (j));
                hp.push_back (hopsInto (j));
                jobs.push_back (j);
            }
            Job seed = base; seed.block = 32; seed.seed ^= 0x1ull; jobs.push_back (seed);
            auto o = renderAll (jobs);
            Tally t;
            const int blocks[] { 441, 512, 4096, 0 };
            for (int k = 1; k <= 4; ++k)
            {
                const int n = vdiff (o[0], o[(size_t) k]);
                const bool same = ev[0]->at == ev[(size_t) k]->at && sameHops (*hp[0], *hp[(size_t) k]) && sameRing (eg25x::an (o[0]).getRing(), eg25x::an (o[(size_t) k]).getRing());
                t.check (n == 0 && same, eg25::blockName (blocks[k - 1]) + juce::String::formatted (": %d diffs%s", n, same ? "" : ", analysis state differs"));
            }
            const auto& d = o[0].eng().diag;
            std::int64_t miss = 0;
            for (auto m : d.followMiss) miss += m;
            bool live = d.onsetEvents > 0 && d.mpmConfident > 0 && d.followHits > 0 && miss > 0 && d.f0Skips > 0 && d.routedDests >= 4 && d.freezeEngages > 0;
            live = live && (tempo ? d.tempoBursts > 0 && d.tempoReanchors > 0 && d.tempoRelocks > 0 : d.freeBursts > 0);
            row (t.bad == 0 && live, juce::String (tempo ? "N9-T Tempo" : "N9-F Free") + ": 441/512/4096/var vs 32 max|d| == 0; events, hops, f0 ring bitwise",
                 t.detail (4) + juce::String::formatted ("  [events %lld bursts %lld confident %lld hits %lld miss %lld skips %lld routed %lld re-anchors %lld re-locks %lld]",
                                                         (long long) d.onsetEvents, (long long) (tempo ? d.tempoBursts : d.freeBursts), (long long) d.mpmConfident,
                                                         (long long) d.followHits, (long long) miss, (long long) d.f0Skips, (long long) d.routedDests,
                                                         (long long) d.tempoReanchors, (long long) d.tempoRelocks) + (live ? "" : "  VACUOUS"));
            row (vdiff (o[0], o[5]) > 0, juce::String (tempo ? "N9c-T" : "N9c-F") + " seed + 1 differs", juce::String (vdiff (o[0], o[5])) + " diffs");
        }
        // N9n — commitHopImmediately (R59) on a tonal follow recipe.
        {
            Job a = followJob (6.0, fromBuffer (steppedSine (48000.0, 6.0, { { 0.0, 233.0 }, { 1.5, 311.0 }, { 3.0, 227.0 }, { 4.5, 262.5 } }, 0.25)), 0.0f,
                               P { { "s1_density", 100.0f } });
            hook (a, [] (auto& h) { h.commitHopImmediately = true; });
            Job b = a;
            a.block = 32; b.block = 441;
            Job c = followJob (6.0, fromBuffer (steppedSine (48000.0, 6.0, { { 0.0, 233.0 }, { 1.5, 311.0 }, { 3.0, 227.0 }, { 4.5, 262.5 } }, 0.25)), 0.0f,
                               P { { "s1_density", 100.0f } }), d2 = c;
            c.block = 32; d2.block = 441;
            auto o = renderAll ({ a, b, c, d2 });
            row (vdiff (o[0], o[1]) > 0 && vdiff (o[2], o[3]) == 0, "N9n commitHopImmediately (the R59 class) breaks the 441 vs 32 null (the tick commit holds it)",
                 juce::String::formatted ("%d diffs with the hook, %d without", vdiff (o[0], o[1]), vdiff (o[2], o[3])));
        }
    }

    void allocRows()
    {
        std::printf ("\n-- M6. PERF-01 allocations across detector events, hops, follow, bursts, abs-set rebuilds\n");
       #if JUCE_MAC
        using namespace eg;
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
        for (int form = 0; form < 2; ++form)
        {
            Job j = n9Base (form == 1, 8.0);
            j.block = 512;
            auto ev = [&j] (juce::int64 at, std::function<void (Proc&)> f) { j.events.push_back ({ at, std::move (f) }); };
            const int major = scaleChoice ("Major");
            ev (40003,  [] (Proc& p) { eg::setP (p, "s1_scale", 5.0f); });                                     // scale change (follow on)
            ev (60011,  [major] (Proc& p) { p.setDegreeMask (2, major, 0x95ull); });                           // mask change
            ev (80021,  [] (Proc& p) { juce::String b; p.setCustomScale ("", "edo 7", b); eg::setP (p, "s3_scale", 31.0f); });   // custom apply
            ev (100003, [] (Proc& p) { eg::setP (p, "mod7_source", (float) ogran::ModSrc::Onset); eg::setP (p, "mod7_dest", (float) D::RevMix);
                                       eg::setP (p, "mod7_depth", 50.0f); });                                  // route add
            ev (120007, [] (Proc& p) { eg::setP (p, "mod1_source", 0.0f); });                                  // route remove
            ev (140009, [] (Proc& p) { eg::setP (p, "mod2_dest", (float) D::stream (0, D::Spray)); });         // route swap
            ev (160001, [] (Proc& p) { eg::setP (p, "onset_threshold", -50.0f); eg::setP (p, "scale_root", 5.0f); });
            ev (200001, [] (Proc& p) { eg::setP (p, "sync_mode", 1.0f); });
            ev (250003, [] (Proc& p) { eg::setP (p, "sync_mode", 0.0f); eg::setP (p, "onset_threshold", -20.0f); eg::setP (p, "scale_root", 11.0f); });
            auto prevAfter = j.after;
            armDuringProcess = false;
            j.after = [prevAfter] (Proc& p, juce::int64 pos) { if (prevAfter) prevAfter (p, pos); armDuringProcess = true; };
            const auto o = render (j);
            armDuringProcess = false;
            const auto& d = o.eng().diag;
            total += allocCount;
            det << (form ? "Tempo" : "Free") << juce::String::formatted (": %d allocs (events %lld, hops %lld, lookups %lld, rebuilds %lld, bursts %lld, route changes %lld); ",
                                                                        (int) allocCount, (long long) d.onsetEvents, (long long) d.mpmHops, (long long) d.followLookups,
                                                                        (long long) d.absSetRebuilds, (long long) (d.freeBursts + d.tempoBursts), (long long) d.routeChanges);
            allocCount = 0;
        }
        malloc_logger = nullptr;
        allocTrace = false;
        row (liveness == 1, "M6 liveness: a deliberate malloc on the render thread counts exactly 1", juce::String (liveness));
        row (liveness == 1 && total == 0, "M6 0 allocations: events, re-phases, Tempo bursts, MPM hops (incl. the first), follow lookups + pass 2, abs-set rebuilds, routes, freeze, automation",
             det.trim());
       #else
        row (false, "M6 alloc gate is macOS-only");
       #endif
    }

    void stressRows()
    {
        std::printf ("\n-- Z6. QUAL-02 analysis stress (60 s x 3 rates) and tap injection\n");
        const double rates[3] { 48000.0, 44100.0, 192000.0 };
        struct ZR { bool finite = true; int sub = 0, scans = 0; double peak = 0.0, wetPeak = 0.0; long long scrubs = 0, nanKills = 0, envScrubs = 0, aScrubs = 0, events = 0, hits = 0, engages = 0; };
        std::vector<ZR> rs (3);
        std::vector<Job> jobs; std::vector<double> cost;
        for (int i = 0; i < 3; ++i)
        {
            const double fs = rates[i];
            Job j; j.fs = fs; j.len = (juce::int64) (60.0 * fs); j.block = 512;
            j.params = eg22::base ({ 1, 2, 3, 4 }) + P { { "grain_limit", 3.0f }, { "mix", 100.0f }, { "feedback", 100.0f }, { "reverb_mix", 20.0f },
                                                       { "onset_threshold", -60.0f }, { "lfo1_shape", 4.0f }, { "lfo1_rate", 20.0f } };
            for (int s = 1; s <= 4; ++s)
                j.params = j.params + P { { eg22::sp (s, "density"), 100.0f }, { eg22::sp (s, "size"), 300.0f }, { eg22::sp (s, "onset_trigger"), 1.0f },
                                          { "pitch_follow_s" + juce::String (s), 1.0f }, { eg22::sp (s, "pitch"), s % 2 ? 24.0f : -24.0f },
                                          { eg22::sp (s, "pitch_rand"), 24.0f }, { eg22::sp (s, "filter_type"), (float) (1 + s % 3) },
                                          { eg22::sp (s, "filter_res"), 70.0f }, { eg22::sp (s, "drive"), 40.0f }, { eg22::sp (s, "scale"), (float) (s == 1 ? 0 : 2 * s) } };
            const int dests[16] { D::stream (0, D::Level), D::stream (1, D::Level), D::stream (2, D::Level), D::stream (3, D::Level),
                                  D::stream (0, D::Pitch), D::stream (1, D::Density), D::Feedback, D::FbSat, D::stream (0, D::FilterRes),
                                  D::stream (1, D::Drive), D::stream (2, D::RingMix), D::RevMix, D::OutputGain, D::Mix,
                                  D::stream (3, D::FilterRes), D::stream (2, D::Pitch) };
            for (int m = 0; m < 16; ++m)
                j.params = j.params + route (m + 1, m % 3 == 2 ? 1 : (m % 2 == 0 ? ogran::ModSrc::Pitch : ogran::ModSrc::Onset), dests[m], m % 2 == 0 ? 100.0f : -100.0f);
            // Execution deviation (stimulus re-derived): the plan's CONTINUOUS 0 dBFS noise
            // masks every burst (+6 dB over the slow envelope never holds: 1 event in 60 s)
            // and gives the tracker nothing confident. Per 250 ms cycle: 60 ms of 0 dBFS
            // noise, 150 ms of a 0.9 sine (110 / 220 / 440 / 880 Hz in turn), 40 ms of
            // silence; 2 ms 0 dBFS 1 kHz bursts every 60 ms on top.
            const double burstPeriod = 0.06;
            j.stim = [fs, burstPeriod] (juce::int64 n, int ch)
            {
                const double t = (double) n / fs, tb = t - burstPeriod * std::floor (t / burstPeriod);
                const double cyc = std::floor (t / 0.25), tc = t - 0.25 * cyc;
                double v = 0.0;
                if (tc < 0.06) v = eg::hashNoise (n, ch, 1.0f);
                else if (tc < 0.21) v = 0.9 * std::sin (2.0 * kPi * 110.0 * std::exp2 ((double) ((juce::int64) cyc % 4)) * t);
                if (tb < 0.002) v += std::sin (2.0 * kPi * 1000.0 * tb);
                return (float) v;
            };
            for (int k = 0; k < 6; ++k)
                j.events.push_back ({ (juce::int64) ((5.0 + 9.0 * k) * fs) + 7, [k] (Proc& p) { eg::setP (p, "freeze", k % 2 == 0 ? 1.0f : 0.0f); } });
            auto* r = &rs[(size_t) i];
            j.after = [r, len = j.len, fs] (Proc& p, juce::int64 pos)
            {
                const juce::int64 sect = (juce::int64) (20.0 * fs);
                if (pos % sect < 512 || pos >= len)
                {
                    const auto& e = p.getEngineForTest();
                    const auto& a = e.getAnalysis();
                    auto sub = [] (float x) { return std::fpclassify (x) == FP_SUBNORMAL; };
                    auto subd = [] (double x) { return std::fpclassify (x) == FP_SUBNORMAL; };
                    for (int k = 0; k < ogran::kPoolLive + ogran::kPoolFade; ++k)
                        if (eg23b::anySubnormal (e.getPool().getGrains()[(size_t) k])) ++r->sub;
                    const auto& det = a.getDetector();
                    int s = (sub (det.hpState()) ? 1 : 0) + (sub (det.fastEnv()) ? 1 : 0) + (sub (det.slowEnv()) ? 1 : 0);
                    for (float x : a.getDecimator().history()) s += sub (x) ? 1 : 0;
                    for (float x : a.getWindow()) s += sub (x) ? 1 : 0;
                    for (int k = 0; k <= a.getMpm().getTauMax() + 1; ++k) s += subd (a.getMpm().nsdfData()[k]) ? 1 : 0;
                    for (float x : a.getGuard().medians()) s += sub (x) ? 1 : 0;
                    s += sub (a.getGuard().accepted()) ? 1 : 0;
                    s += sub (a.getTracker().source()) ? 1 : 0;
                    for (int k = 1; k < ogran::kModSources; ++k) s += sub (e.getSources().v[k]) ? 1 : 0;
                    r->sub += s;
                    ++r->scans;
                }
            };
            jobs.push_back (j);
            cost.push_back (fs);
        }
        eg22b::renderPool (jobs, cost, [&] (size_t i, Out& o)
        {
            auto& r = rs[i];
            for (size_t k = 0; k < o.L.size(); ++k)
            {
                if (! std::isfinite (o.L[k]) || ! std::isfinite (o.R[k])) r.finite = false;
                r.peak = juce::jmax (r.peak, (double) std::abs (o.L[k]), (double) std::abs (o.R[k]));
            }
            const auto& d = o.eng().diag;
            r.scrubs = d.fbScrubs; r.nanKills = d.nanKills; r.envScrubs = d.envScrubs; r.aScrubs = d.analysisScrubs;
            r.wetPeak = d.maxWetPeak; r.events = d.onsetEvents; r.hits = d.followHits; r.engages = d.freezeEngages;
        });
        bool ok = true;
        juce::String det;
        for (int i = 0; i < 3; ++i)
        {
            const auto& r = rs[(size_t) i];
            const double outBound = std::sqrt (1.0 + (double) ogran::kKneeA * ogran::kKneeA) * (double) ogran::dbToGain (12.0f) * (1.0 + 1e-6);
            ok = ok && r.finite && r.sub == 0 && r.wetPeak <= (double) ogran::kKneeA && r.peak <= outBound && r.scrubs == 0 && r.nanKills == 0
                 && r.envScrubs == 0 && r.aScrubs == 0 && r.events > 100 && r.hits > 100 && r.scans >= 3
                 && r.engages >= 3;   // PLAN-4 P149: the freeze arm is live (renderPool honours events)
            det << juce::String::formatted ("%g k: wet %.4f out %.4f sub %d (%d scans) scrubs %lld/%lld/%lld/%lld events %lld hits %lld engages %lld; ", rates[i] / 1000.0, r.wetPeak, r.peak,
                                            r.sub, r.scans, r.scrubs, r.nanKills, r.envScrubs, r.aScrubs, r.events, r.hits, r.engages);
        }
        row (ok, "Z6a 60 s @ 48/44.1/192 k, triggers -60 dB on 0 dBFS noise + bursts, follow +-24 / rand 24, Pitch / Onset +-100 %: finite, no subnormal, bounds, 0 scrubs, freeze engages >= 3", det);

        // Z6b — NaN, +Inf, 1e30 at the tap: isolated.
        {
            const double fs = 48000.0;
            Job base = anaJob (fs, 4.0, eg25x::compositeStim (fs), eg25x::neutralRecipe());
            std::vector<Job> jobs2 { base };
            std::vector<std::shared_ptr<EvLog>> ev { evInto (jobs2[0]) };
            std::vector<std::shared_ptr<HopLog>> hp { hopsInto (jobs2[0]) };
            const float vals[3] { std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), 1.0e30f };
            for (float v : vals)
            {
                Job j = base;
                const juce::int64 at = (juce::int64) (1.3 * fs) + 5;
                hook (j, [v, at] (auto& h) { h.injectAnalysis = { at, v, false }; });
                ev.push_back (evInto (j));
                hp.push_back (hopsInto (j));
                jobs2.push_back (j);
            }
            auto o = renderAll (jobs2);
            Tally t;
            const char* nm[] { "NaN", "+Inf", "1e30" };
            juce::String counts;
            for (int k = 1; k <= 3; ++k)
            {
                bool hopsOk = true;
                for (const auto& h : hp[(size_t) k]->h) hopsOk = hopsOk && (! h.confident || (std::isfinite (h.rawNote) && std::isfinite (h.clarity)));
                bool after = false;
                for (auto e : ev[(size_t) k]->at) after = after || e > (juce::int64) (1.3 * fs);
                const auto dd = eg::diff (o[0], o[(size_t) k]);
                const auto sc = o[(size_t) k].eng().diag.analysisScrubs;
                counts << nm[k - 1] << " " << (long long) sc << "  ";
                t.check (dd.count == 0 && ev[(size_t) k]->at == ev[0]->at && hopsOk && after && sc >= 1,
                         juce::String (nm[k - 1]) + juce::String::formatted (": %d diffs, events %d vs %d, scrubs %lld", dd.count, (int) ev[(size_t) k]->at.size(),
                                                                             (int) ev[0]->at.size(), (long long) sc));
            }
            row (t.bad == 0, "Z6b NaN / +Inf / 1e30 at the tap: scrubbed, no event from them, hops finite or unconfident, the detector fires on the next burst; audio bitwise",
                 t.detail (3) + "  scrubs: " + counts.trim());
        }
    }

    /** timeConfig with a stimulus (tonal + bursts): warm-up 4.5 s, median of 3 × 10 s, % of one core. */
    inline double timeConfigStim (const P& ps, const Stim& st, int& alive, std::vector<double>* blockUs = nullptr, std::vector<int>* blockHop = nullptr, int block = 128)
    {
        const double fs = 48000.0;
        auto p = make();
        eg::setAll (*p, ps);
        p->setSeed (0xC2C2ull);
        p->prepareToPlay (fs, block);
        juce::AudioBuffer<float> buf (2, block);
        juce::MidiBuffer midi;
        juce::int64 pos = 0;
        auto runFor = [&] (double secs, bool perBlock)
        {
            const juce::int64 end = pos + (juce::int64) (secs * fs);
            for (; pos < end; pos += block)
            {
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < block; ++i) buf.setSample (ch, i, st (pos + i, ch));
                const auto hops = p->getEngineForTest().diag.mpmHops;
                const auto t0 = std::chrono::steady_clock::now();
                p->processBlock (buf, midi);
                if (perBlock && blockUs != nullptr)
                {
                    blockUs->push_back (std::chrono::duration<double, std::micro> (std::chrono::steady_clock::now() - t0).count());
                    blockHop->push_back (p->getEngineForTest().diag.mpmHops != hops ? 1 : 0);
                }
            }
        };
        runFor (4.5, false);
        alive = 0;
        for (int s = 0; s < ogran::kPoolLive; ++s) alive += p->getEngineForTest().getPool().isActive (s) ? 1 : 0;
        if (blockUs != nullptr) { runFor (10.0, true); return 0.0; }
        double t[3];
        for (double& x : t)
        {
            const auto t0 = std::chrono::steady_clock::now();
            runFor (10.0, false);
            x = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count() / 10.0;
        }
        std::sort (t, t + 3);
        return t[1] * 100.0;
    }

    void cpuRows()
    {
        std::printf ("\n-- C6. CPU with the analysis + 4 follow streams + triggers (INFO; P77 phase fail > 25 %%)\n");
        P c4 { { "mix", 100.0f }, { "grain_limit", 3.0f }, { "feedback", 100.0f }, { "fb_diffusion", 100.0f }, { "reverb_mix", 50.0f } };
        for (int s = 1; s <= 4; ++s)
            c4 = c4 + P { { eg22::sp (s, "enable"), 1.0f }, { eg22::sp (s, "density"), 16.0f }, { eg22::sp (s, "size"), 2000.0f }, { eg22::sp (s, "window"), 0.6f },
                          { eg22::sp (s, "scale"), 1.0f }, { eg22::sp (s, "pitch_rand"), 2.0f }, { eg22::sp (s, "filter_type"), 1.0f }, { eg22::sp (s, "filter_res"), 100.0f },
                          { eg22::sp (s, "filter_rand"), 100.0f }, { eg22::sp (s, "drive"), 100.0f }, { eg22::sp (s, "crush"), 100.0f }, { eg22::sp (s, "crush_prob"), 100.0f },
                          { eg22::sp (s, "ring_mix"), 100.0f }, { eg22::sp (s, "ring_rand"), 100.0f } };
        P c5 = c4 + P { { "lfo1_rate", 3.0f }, { "lfo2_shape", 5.0f }, { "lfo3_shape", 4.0f }, { "lfo3_rate", 9.0f }, { "lfo4_rate", 0.3f }, { "macro1", 50.0f } };
        const int dests[16] { D::Mix, D::stream (0, D::Position), D::stream (1, D::Pitch), D::stream (2, D::FilterCutoff), D::stream (3, D::Spray),
                              D::Feedback, D::FbSat, D::RevSize, D::stream (0, D::Level), D::stream (1, D::Pan), D::stream (2, D::Size),
                              D::stream (3, D::Density), D::InputGain, D::OutputGain, D::LfoRate1 + 1, D::stream (0, D::RingMix) };
        for (int m = 0; m < 16; ++m)
            c5 = c5 + route (m + 1, 1 + m % 5, dests[m], m % 2 == 0 ? 30.0f : -30.0f);
        P c6 = c5;
        for (int s = 1; s <= 4; ++s)
            c6 = c6 + P { { eg22::sp (s, "onset_trigger"), 1.0f }, { "pitch_follow_s" + juce::String (s), 1.0f } };
        const auto st = toneBurstStim (48000.0, 30.0, 0.18);
        int a5, a6;
        const double c5v = timeConfigStim (c5, st, a5), c6v = timeConfigStim (c6, st, a6);
        std::printf ("  [INFO] C5 re-run (tonal + burst input): %.2f %% of one core (live %d; 2.5a C5 16.26 %% / verify 16.63 %%)\n", c5v, a5);
        std::printf ("  [INFO] C6 = C5 + detector + MPM + 4 follow streams + triggers x4: %.2f %% (live %d), %+.2f vs C5; phase-fail ceiling 25 %%%s\n", c6v, a6,
                     c6v - c5v, c6v > 25.0 ? "  -> PHASE FAIL" : (c6v > 20.0 ? "  -> 20-25 %: flag for 2.6" : ""));
        row (c6v <= 25.0, "C6 CPU <= 25 % (P77 phase-fail ceiling)", juce::String::formatted ("%.2f %%", c6v));
        // C6s — host blocks of 32: blocks with an MPM hop vs blocks without.
        std::vector<double> us;
        std::vector<int> hop;
        int a6s;
        timeConfigStim (c6, st, a6s, &us, &hop, 32);
        std::vector<double> withH, without;
        for (size_t k = 0; k < us.size(); ++k) (hop[k] ? withH : without).push_back (us[k]);
        auto med = [] (std::vector<double> v) { if (v.empty()) return 0.0; std::sort (v.begin(), v.end()); return v[v.size() / 2]; };
        auto mx = [] (const std::vector<double>& v) { double m = 0.0; for (double x : v) m = juce::jmax (m, x); return m; };
        const double excess = med (withH) - med (without);
        std::printf ("  [INFO] C6s 32-sample blocks @ 48 k: hop blocks median %.2f us / worst %.2f us (%d); other blocks median %.2f us / worst %.2f us (%d); "
                     "median hop excess %.2f us (%.1f %% of the 666.7 us budget)%s\n",
                     med (withH), mx (withH), (int) withH.size(), med (without), mx (without), (int) without.size(), excess, excess / 666.7 * 100.0,
                     excess > 66.7 ? "  -> > 10 %: flag for 2.6" : "");
    }

} } // namespace eg25y

static void engineRows25bb()
{
    std::printf ("\n== Stage 2 / 2.5b f0 ring / follow / matrix / PERF rows\n");
    eg25y::capacityRows();
    eg25y::ringRows();
    eg25y::followRows();
    eg25y::absRows();
    eg25y::matrixRows();
    eg25y::burstClickRows();
    eg25y::blockNullRows();
    eg25y::allocRows();
    eg25y::stressRows();
    eg25y::cpuRows();
}
