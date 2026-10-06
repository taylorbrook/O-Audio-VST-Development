/*
   This file is part of O-simpleWavetable, an Ouaricon Audio plugin.
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
/*
  ==============================================================================

    O-simpleWavetable bank-check - Stage 2.1 gates (RESEARCH 5.3).

    Constructs BuiltInBanks directly and measures every table with a float
    juce::dsp::FFT(11): |X[k]| is harmonic k (relative values only).

      G-DIM    5 banks x 32 frames x 11 levels x 2049; sizes exact
      G-PEAK   level-0 sample peak per frame = 0 dB +/- 0.5 dB      (DSP-04)
      G-DC     level-0 mean per frame <= -80 dB                     (DSP-04)
      G-GUARD  s[2048] == s[0] bitwise, every level and frame
      G-SAW    Sine->Saw frame k: h1..hk > -60 dB rel h1; above k <= -120 dB rel max
      G-SQ     Sine->Square: evens <= -60 dB; frame 32 top harmonic = 31; 32 distinct
      G-DRIVE  frame 1 h3 in [-47, -45] dB rel h1; h3, h5, h7 strictly increasing
               over k; evens <= -100 dB rel h1
      G-PULSE  first spectral null at round(1/d_k) +/- 1, frames 1 and 32
      G-FORM   log-F interpolation hits the 5 vowel anchors; 32 distinct frames
      G-BL     level L: max bin in kmax(L)+1..1024 <= -120 dB rel max bin
      G-POL    Saw, Square, Drive frame 1: s[512] > 0
      G-TIME   buildMillis (log only, never gated)
      G-NEG    negative control: a frame built with the Nyquist bin KEPT
               (OSIW_TEST_HOOKS builder flag) must FAIL G-BL at L0; the same
               spectrum through the normal builder must pass.

    One line per gate: "PASS G-x ..." / "FAIL G-x: ...", plus measured values.
    Exit 0 only if every gate passes. Off by default; -DOUARICON_BUILD_TESTS=ON.

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_events/juce_events.h>

#include "BankFactory.h"
#include "BuiltInBanks.h"
#include "MipmapBuilder.h"
#include "WavetableBank.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    constexpr int kN       = WavetableBank::kTableSize;   // 2048
    constexpr int kNyq     = kN / 2;                      // 1024
    constexpr int kLevels  = WavetableBank::kLevels;
    constexpr int kFrames  = BuiltInBanks::kFrames;

    int gFailures = 0;

    void report (const char* gate, bool ok, const std::string& detail)
    {
        std::cout << (ok ? "PASS " : "FAIL ") << gate << (ok ? " " : ": ") << detail << "\n";
        std::cout.flush();
        if (! ok)
            ++gFailures;
    }

    std::string fmt (double v, int prec = 3)
    {
        std::ostringstream s;
        s << std::fixed << std::setprecision (prec) << v;
        return s.str();
    }

    std::string fmtSci (double v)
    {
        std::ostringstream s;
        s << std::scientific << std::setprecision (3) << v;
        return s.str();
    }

    double dB (double ratio)
    {
        return 20.0 * std::log10 (std::max (ratio, 1.0e-300));
    }

    //==========================================================================
    // |X[k]|, k = 0..1024, of one 2048-sample cycle.
    struct Analyser
    {
        juce::dsp::FFT fft { 11 };
        std::vector<float> buf = std::vector<float> ((size_t) (2 * kN), 0.0f);
        std::vector<double> mag = std::vector<double> ((size_t) (kNyq + 1), 0.0);

        const std::vector<double>& run (const float* x)
        {
            std::fill (buf.begin(), buf.end(), 0.0f);
            std::copy (x, x + kN, buf.begin());
            fft.performRealOnlyForwardTransform (buf.data());
            for (int k = 0; k <= kNyq; ++k)
            {
                const double re = buf[(size_t) (2 * k)];
                const double im = buf[(size_t) (2 * k + 1)];
                mag[(size_t) k] = std::sqrt (re * re + im * im);
            }
            return mag;
        }
    };

    double maxBin (const std::vector<double>& m, int from, int to)
    {
        double best = 0.0;
        for (int k = from; k <= to; ++k)
            best = std::max (best, m[(size_t) k]);
        return best;
    }

    // Worst (highest) dB of bins kmax(L)+1..1024 relative to the max bin.
    double aboveKmaxDb (Analyser& an, const float* table, int level)
    {
        const auto& m = an.run (table);
        const double peak = maxBin (m, 1, kNyq);
        const double above = maxBin (m, WavetableBank::kmax (level) + 1, kNyq);
        return dB (above / std::max (peak, 1.0e-300));
    }

    bool framesDistinct (const WavetableBank& b, int level, int& distinctOut)
    {
        int distinct = 0;
        for (int f = 0; f < b.numFrames; ++f)
        {
            bool unique = true;
            for (int g = 0; g < f; ++g)
                if (std::memcmp (b.frame (level, f), b.frame (level, g), sizeof (float) * (size_t) WavetableBank::kStride) == 0)
                    unique = false;
            if (unique)
                ++distinct;
        }
        distinctOut = distinct;
        return distinct == b.numFrames;
    }

    //==========================================================================
    void gateDim (const BuiltInBanks& banks)
    {
        bool ok = true;
        std::string d;
        const size_t wantData = (size_t) kLevels * (size_t) kFrames * (size_t) WavetableBank::kStride;
        const size_t wantThumbs = (size_t) kFrames * (size_t) WavetableBank::kThumbSize;
        for (int i = 0; i < BuiltInBanks::kCount; ++i)
        {
            const auto* b = banks.get (i);
            if (b == nullptr) { ok = false; d += "bank " + std::to_string (i) + " null; "; continue; }
            d += std::string (b->name.toRawUTF8()) + ": frames=" + std::to_string (b->numFrames)
               + " data=" + std::to_string (b->data.size()) + " thumbs=" + std::to_string (b->thumbs.size()) + "; ";
            if (b->numFrames != kFrames || b->data.size() != wantData || b->thumbs.size() != wantThumbs)
                ok = false;
        }
        if (banks.get (5) != nullptr || banks.get (-1) != nullptr)
        {
            ok = false;
            d += "get() out of range not null; ";
        }
        report ("G-DIM", ok, d + "want data=" + std::to_string (wantData));
    }

    void gatePeakDcGuard (const BuiltInBanks& banks)
    {
        double worstPeakDev = 0.0, worstDc = -1000.0;
        int guardMismatch = 0;
        std::string perBank;
        for (int i = 0; i < BuiltInBanks::kCount; ++i)
        {
            const auto& b = *banks.get (i);
            double bankWorstPeak = 0.0, bankWorstDc = -1000.0;
            for (int f = 0; f < b.numFrames; ++f)
            {
                const float* s = b.frame (0, f);
                double peak = 0.0, sum = 0.0;
                for (int n = 0; n < kN; ++n)
                {
                    peak = std::max (peak, (double) std::abs (s[n]));
                    sum += (double) s[n];
                }
                const double dev = std::abs (dB (peak));
                const double dc  = dB (std::abs (sum / (double) kN));
                bankWorstPeak = std::max (bankWorstPeak, dev);
                bankWorstDc   = std::max (bankWorstDc, dc);

                for (int level = 0; level < kLevels; ++level)
                {
                    const float* t = b.frame (level, f);
                    if (std::memcmp (&t[kN], &t[0], sizeof (float)) != 0)
                        ++guardMismatch;
                }
            }
            worstPeakDev = std::max (worstPeakDev, bankWorstPeak);
            worstDc      = std::max (worstDc, bankWorstDc);
            perBank += std::string (b.name.toRawUTF8()) + " |peak dev| " + fmt (bankWorstPeak, 6)
                     + " dB, DC " + fmt (bankWorstDc, 1) + " dB; ";
        }
        report ("G-PEAK", worstPeakDev <= 0.5, "worst |peak - 0 dB| = " + fmt (worstPeakDev, 6) + " dB (<= 0.5); " + perBank);
        report ("G-DC",   worstDc <= -80.0,    "worst level-0 mean = " + fmt (worstDc, 1) + " dB (<= -80)");
        report ("G-GUARD", guardMismatch == 0, "guard mismatches = " + std::to_string (guardMismatch)
                                                + " over " + std::to_string (5 * kFrames * kLevels) + " tables");
    }

    void gateSaw (const BuiltInBanks& banks, Analyser& an)
    {
        const auto& b = *banks.get (BankFactory::sineSaw);
        double worstIn = 0.0, worstAbove = -1000.0;
        bool ok = true;
        for (int k = 1; k <= kFrames; ++k)
        {
            const auto& m = an.run (b.frame (0, k - 1));
            const double h1 = m[1], peak = maxBin (m, 1, kNyq);
            double minIn = 0.0;
            for (int n = 1; n <= k; ++n)
                minIn = std::min (minIn, dB (m[(size_t) n] / h1));
            const double above = dB (maxBin (m, k + 1, kNyq) / peak);
            if (! (minIn > -60.0) || ! (above <= -120.0))
                ok = false;
            worstIn = std::min (worstIn, minIn);
            worstAbove = std::max (worstAbove, above);
        }
        report ("G-SAW", ok, "weakest in-band harmonic " + fmt (worstIn, 2) + " dB rel h1 (> -60); worst above-k "
                             + fmt (worstAbove, 1) + " dB rel max (<= -120); frame 1 = pure sine by the same bound");
    }

    void gateSquare (const BuiltInBanks& banks, Analyser& an)
    {
        const auto& b = *banks.get (BankFactory::sineSquare);
        double worstEven = -1000.0;
        for (int f = 0; f < b.numFrames; ++f)
        {
            const auto& m = an.run (b.frame (0, f));
            const double peak = maxBin (m, 1, kNyq);
            for (int n = 2; n <= kNyq; n += 2)
                worstEven = std::max (worstEven, dB (m[(size_t) n] / peak));
        }

        const auto& m32 = an.run (b.frame (0, kFrames - 1));
        const double peak32 = maxBin (m32, 1, kNyq);
        int top = 0;
        for (int n = 1; n <= kNyq; ++n)
            if (dB (m32[(size_t) n] / peak32) > -100.0)
                top = n;

        int distinct = 0;
        const bool allDistinct = framesDistinct (b, 0, distinct);

        report ("G-SQ", worstEven <= -60.0 && top == 31 && allDistinct,
                "worst even harmonic " + fmt (worstEven, 1) + " dB (<= -60); frame 32 top harmonic h"
                + std::to_string (top) + " (want h31); distinct frames " + std::to_string (distinct) + "/32");
    }

    void gateDrive (const BuiltInBanks& banks, Analyser& an)
    {
        const auto& b = *banks.get (BankFactory::drive);
        std::vector<double> h3, h5, h7;
        double worstEven = -1000.0;
        for (int f = 0; f < b.numFrames; ++f)
        {
            const auto& m = an.run (b.frame (0, f));
            const double h1 = m[1];
            h3.push_back (dB (m[3] / h1));
            h5.push_back (dB (m[5] / h1));
            h7.push_back (dB (m[7] / h1));
            for (int n = 2; n <= kNyq; n += 2)
                worstEven = std::max (worstEven, dB (m[(size_t) n] / h1));
        }

        auto strictlyUp = [] (const std::vector<double>& v)
        {
            for (size_t i = 1; i < v.size(); ++i)
                if (! (v[i] > v[i - 1]))
                    return false;
            return true;
        };

        const bool f1ok = h3[0] >= -47.0 && h3[0] <= -45.0;
        const bool mono = strictlyUp (h3) && strictlyUp (h5) && strictlyUp (h7);
        report ("G-DRIVE", f1ok && mono && worstEven <= -100.0,
                "frame 1 h3 " + fmt (h3[0], 2) + " dB (in [-47,-45]); frame 32 h3/h5/h7 "
                + fmt (h3.back(), 2) + "/" + fmt (h5.back(), 2) + "/" + fmt (h7.back(), 2)
                + " dB; h3,h5,h7 strictly increasing: " + (mono ? "yes" : "NO")
                + "; worst even " + fmt (worstEven, 1) + " dB rel h1 (<= -100)");
    }

    void gatePulse (const BuiltInBanks& banks, Analyser& an)
    {
        const auto& b = *banks.get (BankFactory::pulseWidth);
        bool ok = true;
        std::string d;
        for (const int k : { 1, kFrames })
        {
            const auto& m = an.run (b.frame (0, k - 1));
            const double peak = maxBin (m, 1, kNyq);
            int firstNull = -1;
            for (int n = 2; n <= kNyq; ++n)
                if (m[(size_t) n] < peak * 1.0e-4)   // -80 dB rel max
                {
                    firstNull = n;
                    break;
                }
            const int want = (int) std::lround (1.0 / BankFactory::pulseDuty (k));
            if (std::abs (firstNull - want) > 1)
                ok = false;
            d += "frame " + std::to_string (k) + ": first null h" + std::to_string (firstNull)
               + " (want h" + std::to_string (want) + " +/- 1); ";
        }
        report ("G-PULSE", ok, d);
    }

    void gateFormant (const BuiltInBanks& banks)
    {
        bool anchorsOk = true;
        double worstRel = 0.0;
        for (int v = 0; v < BankFactory::kNumVowels; ++v)
        {
            const auto want = BankFactory::vowelAnchor (v);
            const auto got  = BankFactory::formantsAt (BankFactory::vowelAnchorFrame (v));
            const double rel = std::max ({ std::abs (got.f1 - want.f1) / want.f1,
                                           std::abs (got.f2 - want.f2) / want.f2,
                                           std::abs (got.f3 - want.f3) / want.f3 });
            worstRel = std::max (worstRel, rel);
            if (! (rel <= 1.0e-9))
                anchorsOk = false;
        }

        // Non-vacuity: a mid-segment frame is strictly between its anchors (log-F).
        const auto mid = BankFactory::formantsAt (4.875);   // halfway A -> E
        const auto a = BankFactory::vowelAnchor (0), e = BankFactory::vowelAnchor (1);
        const double wantF1 = std::sqrt (a.f1 * e.f1);
        const bool midOk = std::abs (mid.f1 - wantF1) / wantF1 <= 1.0e-9;

        int distinct = 0;
        const bool allDistinct = framesDistinct (*banks.get (BankFactory::formant), 0, distinct);

        report ("G-FORM", anchorsOk && midOk && allDistinct,
                "anchor worst rel error " + fmtSci (worstRel)
                + " (<= 1e-9); mid A-E F1 " + fmt (mid.f1, 3) + " Hz (geo mean " + fmt (wantF1, 3)
                + "); distinct frames " + std::to_string (distinct) + "/32");
    }

    void gateBandLimit (const BuiltInBanks& banks, Analyser& an)
    {
        double worstAll = -1000.0;
        std::string d;
        for (int i = 0; i < BuiltInBanks::kCount; ++i)
        {
            const auto& b = *banks.get (i);
            double worstBank = -1000.0;
            int worstLevel = -1, worstFrame = -1;
            for (int level = 0; level < kLevels; ++level)
                for (int f = 0; f < b.numFrames; ++f)
                {
                    const double v = aboveKmaxDb (an, b.frame (level, f), level);
                    if (v > worstBank) { worstBank = v; worstLevel = level; worstFrame = f + 1; }
                }
            worstAll = std::max (worstAll, worstBank);
            d += std::string (b.name.toRawUTF8()) + " " + fmt (worstBank, 1) + " dB (L" + std::to_string (worstLevel)
               + " frame " + std::to_string (worstFrame) + "); ";
        }
        report ("G-BL", worstAll <= -120.0, "worst above-kmax " + fmt (worstAll, 1) + " dB rel max (<= -120); " + d);
    }

    void gatePolarity (const BuiltInBanks& banks)
    {
        bool ok = true;
        std::string d;
        for (const int i : { (int) BankFactory::sineSaw, (int) BankFactory::sineSquare, (int) BankFactory::drive })
        {
            const auto& b = *banks.get (i);
            const float v = b.frame (0, 0)[512];
            if (! (v > 0.0f))
                ok = false;
            d += std::string (b.name.toRawUTF8()) + " s[512]=" + fmt (v, 5) + "; ";
        }
        report ("G-POL", ok, d);
    }

    void gateNegative (Analyser& an)
    {
        // h1 sine + a Nyquist component (cosine amplitude 0.01 -> X[1024] = N * 0.01).
        std::vector<float> spec ((size_t) MipmapBuilder::kPacked, 0.0f);
        spec[3] = -0.5f * (float) kN;
        spec[(size_t) (2 * kNyq)] = (float) kN * 0.01f;

        WavetableBank kept, clean;
        kept.allocate (1);
        clean.allocate (1);

        MipmapBuilder builder;
        builder.testKeepNyquist = true;
        builder.buildFromSpectrum (kept, 0, spec.data(), MipmapBuilder::Normalise::none);
        builder.testKeepNyquist = false;
        builder.buildFromSpectrum (clean, 0, spec.data(), MipmapBuilder::Normalise::none);

        const double keptDb  = aboveKmaxDb (an, kept.frame (0, 0), 0);
        const double cleanDb = aboveKmaxDb (an, clean.frame (0, 0), 0);
        const bool keptFails  = ! (keptDb <= -120.0);
        const bool cleanPasses = cleanDb <= -120.0;

        report ("G-NEG", keptFails && cleanPasses,
                "Nyquist kept: L0 above-kmax " + fmt (keptDb, 1) + " dB -> G-BL "
                + (keptFails ? "FAILS (as designed)" : "PASSES (measurer blind!)")
                + "; normal builder: " + fmt (cleanDb, 1) + " dB -> " + (cleanPasses ? "passes" : "FAILS"));
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    BuiltInBanks banks;
    Analyser an;

    std::cout << "bank-check: " << BuiltInBanks::kCount << " banks, buildMillis = "
              << fmt (banks.buildMillis, 2) << " ms\n";

    gateDim (banks);
    gatePeakDcGuard (banks);
    gateSaw (banks, an);
    gateSquare (banks, an);
    gateDrive (banks, an);
    gatePulse (banks, an);
    gateFormant (banks);
    gateBandLimit (banks, an);
    gatePolarity (banks);
    report ("G-TIME", true, "buildMillis = " + fmt (banks.buildMillis, 2) + " ms (log only, never gated)");
    gateNegative (an);

    std::cout << (gFailures == 0 ? "bank-check: ALL PASS\n" : "bank-check: FAILURES = ")
              << (gFailures == 0 ? std::string() : std::to_string (gFailures) + "\n");
    return gFailures == 0 ? 0 : 1;
}
