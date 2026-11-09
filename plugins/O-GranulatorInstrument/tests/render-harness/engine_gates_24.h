/*
  ==============================================================================

    O-GranulatorInstrument: render harness, Stage 2 / Phase 2.4 engine gates
    (PLAN-2.4 T3 / T12: working-copy feedback, Freeze, Reset, reverb + knee).
    Included by main.cpp AFTER engine_gates_23.h (uses eg21 / eg22 / eg23
    helpers and rh::).

    Every row prints: ID, measured value, threshold, margin and its negative
    control ("ctrl FIRED: ..." / "ctrl NOT FIRED (vacuous): ..."); a row whose
    control does not fire FAILS.

      TB0  shipping renders == renders-2.4 (14 jobs)        ctrl N1-default + legacyWetTanh
      TB2  knee identity <= kKneeT; |out| <= |pre| <= ...;   ctrl legacyWetTanh -> identity-half renders differ
           |out| <= float (A) above T (FZ1 48 k)

  ==============================================================================
*/
#pragma once

namespace { namespace eg24
{
    using eg21::Session;
    using eg21::P;
    using eg21::with;
    using eg21::setP;
    using eg21::f2;
    using eg21::i64;
    using eg21::ctrl;
    using eg21::sameF;
    using eg21::noiseBuf;
    using eg21::sineBuf;
    using eg21::kSeed;
    using eg22::base4;
    using eg22::all4;
    using eg22::cat;
    using eg22::sn;
    using eg22::shipping;

    constexpr double kFs = 48000.0;
    using Proc = GranulatorInstrumentProcessor;

    void section (const char* t) { std::printf ("\n-- %s\n", t); }

    /** "<job> <hex>" golden table → map. */
    std::map<juce::String, juce::String> readTable (const juce::String& file)
    {
        std::map<juce::String, juce::String> m;
        juce::StringArray g;
        g.addLines (gGoldenDir.getChildFile (file).loadFileAsString());
        for (const auto& l : g)
            if (l.trim().isNotEmpty())
                m[l.upToFirstOccurrenceOf (" ", false, false)] = l.fromLastOccurrenceOf (" ", false, false).trim();
        return m;
    }

    std::map<juce::String, juce::String> toMap (const juce::StringArray& lines)
    {
        std::map<juce::String, juce::String> m;
        for (const auto& l : lines)
            m[l.upToFirstOccurrenceOf (" ", false, false)] = l.fromLastOccurrenceOf (" ", false, false).trim();
        return m;
    }

    /** Lines of `now` equal to `gold` (same key, same hash). */
    int sameLines (const std::map<juce::String, juce::String>& now, const std::map<juce::String, juce::String>& gold)
    {
        int n = 0;
        for (const auto& kv : now)
        {
            const auto it = gold.find (kv.first);
            n += (it != gold.end() && it->second == kv.second) ? 1 : 0;
        }
        return n;
    }

    double peakOf (const rh::JobOut& o)
    {
        double p = 0.0;
        for (float x : o.L) p = std::max (p, (double) std::abs (x));
        for (float x : o.R) p = std::max (p, (double) std::abs (x));
        return p;
    }

    //==========================================================================
    // TB0 — the true-bypass invariant from PLAN-2.4 T4 on: shipping == renders-2.4
    void rowTB0()
    {
        const auto gold = readTable ("renders-2.4.sha256");
        const auto now  = toMap (rh::computeAll());
        const int same = sameLines (now, gold);

        // Control: N1-default under legacyWetTanh hashes differently from renders-2.4.
        juce::String hC;
        {
            rh::Overrides o = rh::overrides;
            o.legacy = true;
            rh::ScopedOverrides so (o);
            hC = rh::sha (rh::jobN1 (false, kSeed));
        }
        const auto it = gold.find ("N1-default");
        const bool fired = it != gold.end() && hC.isNotEmpty() && hC != it->second;
        row (gold.size() == 14 && now.size() == 14 && same == 14 && fired,
             "TB0 shipping renders == renders-2.4.sha256 (14 jobs, PLAN-2.4 PD1)",
             "meas " + juce::String (same) + "/14 lines identical (golden " + juce::String ((int) gold.size()) + " lines) | thr 14 | margin "
                 + juce::String (same - 14) + " | " + ctrl (fired, "N1-default + legacyWetTanh -> hash differs"));
    }

    //==========================================================================
    // TB2 — the knee is identity at or below kKneeT (R§6.2)
    struct StressOut { std::vector<float> L, R; };

    StressOut stressRender (double fs, double secs, bool wetOff, bool legacy)
    {
        Session s (fs, eg23::stressParams(), kSeed, 512);
        shipping (s);
        s.hk().wetSafetyOff  = wetOff;
        s.hk().legacyWetTanh = legacy;
        const int n = (int) (2.0 * fs);
        s.load (0, noiseBuf (2, n, 1.0f, 41), fs);
        s.load (1, noiseBuf (2, n, 1.0f, 42), fs);
        s.load (2, sineBuf (2, n, 1000.0, fs, 1.0), fs);
        s.load (3, sineBuf (2, n, 1000.0, fs, 1.0), fs);
        for (int k = 0; k < 16; ++k)
            s.noteOn (0, 84 + k);
        s.run ((juce::int64) (secs * fs), 512);
        return { s.L, s.R };
    }

    void rowTB2()
    {
        // Identity half: N1-default and P2-default, shipping vs wetSafetyOff (bitwise).
        // The wetSafetyOff render's peak must sit at or below kKneeT (else one
        // recorded step: s1_level −6 dB on every render; still above → INVALID).
        auto renderPair = [] (const P& extra, bool legacy, rh::JobOut (&ship)[2], rh::JobOut (&off)[2])
        {
            for (int w = 0; w < 2; ++w)
            {
                rh::Overrides o = rh::overrides;
                o.extra = extra;
                o.legacy = legacy;
                o.wetOff = (w == 1);
                rh::ScopedOverrides so (o);
                auto& dst = (w == 0) ? ship : off;
                dst[0] = rh::jobN1 (false, kSeed);
                dst[1] = rh::jobP2 (false);
            }
        };

        P extra;
        rh::JobOut ship[2], off[2];
        renderPair (extra, false, ship, off);
        double pk = std::max (peakOf (off[0]), peakOf (off[1]));
        juce::String step = "none";
        if (pk > (double) ogi::kKneeT)
        {
            extra = { { eg21::s1 ("level"), -6.0f } };
            renderPair (extra, false, ship, off);
            pk = std::max (peakOf (off[0]), peakOf (off[1]));
            step = "s1_level -6 dB (recorded)";
        }
        const bool valid = pk <= (double) ogi::kKneeT && pk > 0.0;
        const bool idN1 = rh::sha (ship[0]) == rh::sha (off[0]);
        const bool idP2 = rh::sha (ship[1]) == rh::sha (off[1]);

        // Control: the identity-half renders under legacyWetTanh differ from wetSafetyOff.
        rh::JobOut lship[2], loff[2];
        renderPair (extra, true, lship, loff);
        const bool fired = rh::sha (lship[0]) != rh::sha (off[0]) && rh::sha (lship[1]) != rh::sha (off[1]);

        // Knee half: the FZ1 stimulus at 48 k, 2 s: knee render vs wetSafetyOff (pre).
        const auto pre = stressRender (kFs, 2.0, true, false);
        const auto out = stressRender (kFs, 2.0, false, false);
        std::uint64_t idBad = 0, bound = 0, notBelow = 0, nId = 0, nKnee = 0, nEqual = 0;
        double prePk = 0.0, outPk = 0.0;
        const size_t N = std::min (pre.L.size(), out.L.size());
        for (int ch = 0; ch < 2; ++ch)
        {
            const auto& a = ch == 0 ? pre.L : pre.R;
            const auto& b = ch == 0 ? out.L : out.R;
            for (size_t i = 0; i < N; ++i)
            {
                const float x = a[i], y = b[i];
                prePk = std::max (prePk, (double) std::abs (x));
                outPk = std::max (outPk, (double) std::abs (y));
                if (std::abs (y) > ogi::kKneeA) ++bound;
                if (std::abs (x) <= ogi::kKneeT)
                {
                    ++nId;
                    if (! sameF (x, y)) ++idBad;
                }
                else
                {
                    ++nKnee;
                    // The knee property is y <= x (OutputLaw.h): just above T,
                    // tanh (u) ~ u rounds back to x in float (PLAN-2.4 deviation:
                    // the plan's strict < is false there; equal count is INFO).
                    if (! (std::abs (y) <= std::abs (x))) ++notBelow;
                    if (std::abs (y) == std::abs (x)) ++nEqual;
                }
            }
        }
        const bool kneeValid = prePk > 0.6 && nKnee > 0;
        const bool ok = valid && idN1 && idP2 && fired && kneeValid && idBad == 0 && bound == 0 && notBelow == 0;
        row (ok, "TB2 knee identity <= kKneeT (N1/P2 default bitwise == wetSafetyOff) + knee bound (FZ1 48 k)",
             "meas identity N1 " + juce::String (idN1 ? "==" : "!=") + ", P2 " + juce::String (idP2 ? "==" : "!=")
                 + " (wetOff peak " + f2 (pk, 4) + ", step " + step + (valid ? "" : ", INVALID") + ")"
                 + "; knee half: " + i64 (nId) + " samples |pre|<=T, " + i64 (idBad) + " differ; " + i64 (nKnee) + " above T, "
                 + i64 (notBelow) + " with |out| > |pre| (INFO " + i64 (nEqual) + " equal, rounding near T); |out| > A: " + i64 (bound) + " (pre peak " + f2 (prePk, 3) + ", out peak "
                 + f2 (outPk, 6) + (kneeValid ? "" : ", INVALID pre <= 0.6") + ") | thr 0 / 0 / 0, out <= " + f2 (ogi::kKneeA, 8)
                 + " | margin " + f2 ((double) ogi::kKneeA - outPk, 6)
                 + " | " + ctrl (fired, "legacyWetTanh -> N1 / P2 identity-half renders differ"));
    }

    void runAll()
    {
        const double t0 = juce::Time::getMillisecondCounterHiRes();
        std::printf ("\n== Phase 2.4 engine gates (feedback, freeze, reset, reverb + knee)\n");
        section ("TB. true bypass + knee");
        rowTB0();
        rowTB2();
        std::printf ("\n-- Phase 2.4 engine gates done (%.1f s)\n", (juce::Time::getMillisecondCounterHiRes() - t0) * 0.001);
    }
}} // namespace ::eg24
