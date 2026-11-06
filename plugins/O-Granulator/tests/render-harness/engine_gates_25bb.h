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

} } // namespace eg25y

static void engineRows25bb()
{
    std::printf ("\n== Stage 2 / 2.5b f0 ring / follow / matrix / PERF rows\n");
    eg25y::capacityRows();
    eg25y::ringRows();
}
