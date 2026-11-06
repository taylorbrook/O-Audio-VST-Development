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

} } // namespace eg25y

static void engineRows25bb()
{
    std::printf ("\n== Stage 2 / 2.5b f0 ring / follow / matrix / PERF rows\n");
    eg25y::capacityRows();
    eg25y::ringRows();
    eg25y::followRows();
    eg25y::absRows();
}
