/*
  ==============================================================================

    O-Granulator — render harness, Stage 2 / Phase 2.6 hardening rows
    (PLAN-2.6 P94). Sibling: engine_gates_26b.h.

      TA1          D21 infinite tail, latency 0                          (T1)
      Z7s          census self-test + reach (R93, R94, P83)              (T2)
      Z7h1–5, Z7hn, Z7hr   R96 host non-finite input (ring + wet guard)  (T3)
      Z7a / Z7m1–8 / Z7f   QUAL-02 extremes, min-side hazards, fuzz      (T8)
      Z7d / Z7dn   decay to 340 s, FTZ on / off (`--long`)               (T8)
      HR1–HR3      96 / 192 k smoke (R101, R102)                         (T10)

  ==============================================================================
*/
#pragma once

namespace { namespace eg26
{
    using eg::Job;
    using eg::Out;
    using eg::Proc;
    using eg::P;

    inline void info (const juce::String& s) { std::printf ("  [INFO] %s\n", s.toRawUTF8()); }

    //==========================================================================
    // TA1 — D21 infinite tail (T1)
    //==========================================================================
    void tailRows()
    {
        std::printf ("\n-- TA. D21 tail length + latency\n");
        auto p = make();
        const double t0 = p->getTailLengthSeconds();
        p->prepareToPlay (48000.0, 512);
        const double t1 = p->getTailLengthSeconds();
        auto inf = [] (double t) { return std::isinf (t) && t > 0.0; };
        row (inf (t0) && inf (t1) && p->getLatencySamples() == 0, "TA1 getTailLengthSeconds() == +inf before and after prepareToPlay (48 k / 512); latency 0",
             juce::String::formatted ("tail %g / %g, latency %d", t0, t1, p->getLatencySamples()));
    }
} } // namespace eg26

static void engineRows26()
{
    std::printf ("\n== Stage 2 / 2.6 hardening rows (tail, census, host input, stress, high rate)\n");
    eg26::tailRows();
}
