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
} } // namespace eg25y

static void engineRows25bb()
{
    std::printf ("\n== Stage 2 / 2.5b f0 ring / follow / matrix / PERF rows\n");
    eg25y::capacityRows();
}
