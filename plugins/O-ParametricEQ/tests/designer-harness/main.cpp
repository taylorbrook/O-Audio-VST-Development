/*
   This file is part of the Ouaricon Audio plugin suite.
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

    O-ParametricEQ - designer harness (Stage 2, Phases 2.1 + 2.1b; offline gate G1)

      O-ParametricEQ-designer-harness                      run every row
      O-ParametricEQ-designer-harness --golden-dir <dir>   compare D1 / D2 against <dir>/designs.tsv
                                                           and <dir>/designs-polished.tsv
      O-ParametricEQ-designer-harness --dump-designs       print the golden's settings list as
                                                           designed by the C++ seed, exit 0
      O-ParametricEQ-designer-harness --dump-polished      the same for the polished golden's list
                                                           (inspection only: another solver than
                                                           the model's, not byte-identical)
      O-ParametricEQ-designer-harness --neg <name>         run ONE negative control; exit 0 only
                                                           if the targeted row FAILS. Names:
                                                           rbj, f1, alloc, seed, rbj-b, binary-gate,
                                                           first-build, engine-alloc (exit 0 only
                                                           if the planted allocation is counted)

    Exit code = number of FAIL rows (capped at 255). Last line: "N PASS, M FAIL, K INFO".

    Plain C++17: the Source/dsp headers and the standard library. The goldens
    (tests/golden/designs.tsv, designs-polished.tsv) are written by
    tests/model/dump_designs.py and dump_polish.py, never by this program: the
    --dump options go to stdout for comparison only.

    Every Phase 2.1 row below runs Designer::designSeed (the closed form). The
    Phase 2.1b rows run Designer::design (polished) unless they say otherwise.

      Phase 2.2 (PLAN-2.2 T4): the engine in double, eq::Band + eq::Router driven
      by Designer::design, settings through eq::makeBandInput
      EN-ir      impulse response vs eq::bandMagDb, eight types x five rates (1e-6 dB above
                 -80 dB; tail 200 dB down, asserted)
      EN-irn     the bell against the closed form of the bell 1 % higher must exceed 1e-6 dB
      EN-ms      Mid / Side bands: the other component within -250 dB re input peak
      EN-msn     EN-ms with the bands on Left must fail
      EN-lr      Left / Right bands: the other lane memcmp equal
      EN-g8      10 000 random configurations + three forced High Cuts: output starts at the
                 impulse index wherever the prediction is non-zero; y[0] vs the predicted
                 first-sample gain within 1e-12 of the chain's largest magnitude (G8 as amended)
      EN-blk     block-size invariance, bit for bit
      EN-fin     finite output above Nyquist and under random full-range settings
      EN-alloc   no allocation in setDesign + Router processing (malloc_logger)
      SN1-SN3    makeBandInput: sanitising, exact gain at scale 100, canonical form
      EN-dyn     the dynamic offset argument (DSP-07, engine half)

      Phase 2.1b (PLAN-2.1b T6)
      D2         C++ design vs designs-polished.tsv (1e-6 / 1e-9 relative, response 1e-6 dB)
      G1b-bell   G1 at 0.05 + 0.012 |g| (0.03 + 0.002 |g| from 88.2 kHz), bells
      G1b-sh12   the same, 12 dB/oct shelves, Q <= 1.41
      G1b-shN    the same, 24 / 36 / 48 / 72 / 96 dB/oct shelves, Q 1
      G1b-sh6    6 dB/oct shelves at the unchanged 2.1 tolerance
      G1bn-seed  the seed scored by the G1b-bell code must FAIL
      G1bn-rbj   the RBJ bell scored by the G1b-bell code must FAIL
      NW1        per section: polished error <= seed error + 1e-3 dB
      R2         reciprocity, polished
      C1, C1n    continuity on lines through parameter space (4x of the seed, shrinking 2x per
                 4x steps); a binary extension gate must FAIL on a line the shipped design passes
      C1-hi      C1 on lines at 88.2 / 96 / 192 kHz
      NW2        random full range, per section: polished error <= seed error + 1.0 dB
      RA1        on NW2's draw: no accepted section with g or k more than 10x from the seed's
      PR1        below 44.1 kHz design == designSeed bit for bit, every weight 0
      P1s        shelf re-centre: continuity at its boundaries, region, moved pairs (listed
                 with P1; scores designSeed and buildPrototype)
      F2         F1 with design; gain2 parameters strictly positive; weight in [0, 1]
      S1         where the weight is 0, design == designSeed bit for bit
      TL2        tilt error == high-shelf error
      A2, B2     A1 and B1 with design
      H1, I-odd, I-weak, I-new, I-cost   INFO

      Hardening carried from VERIFICATION-2.1
      G2s-short  High Cut cells without a -50 dB point: slope against the prototype's
      CS1, CS2   cut-skirt regression ceilings
      P1         two-sided clamp-frequency check with harness-local literals

      Phase 2.1
      D1       C++ design of every golden line vs designs.tsv (1e-9 relative)
      E1       ResponseEval vs the SVF / one-pole recurrences run on an impulse
      G1-bell  G1, bells, five rates
      G1-sh12  G1, 12 dB/oct shelves, Q <= 1.0, five rates
      G1-sh6   G1, 6 dB/oct shelves, five rates
      G1n      an RBJ bilinear bell scored by the G1-bell code must FAIL
      T1       the eleven "Model results" rows within 0.05 dB
      R1       boost / cut reciprocity within 1e-9 dB
      N1       10 Hz, Q 40, +30 dB bell at 192 kHz within 1e-9 dB
      F1       1e6 random settings x 8 rates: finite, positive, section count
      F1n      the F1 checker FAILS a NaN and a -1
      G2s      cut slopes and corner gains (G2 static part)
      SL1      D6: slope inert on Bell / Notch / Band Pass
      Q1       D9: Q inert at 6 dB/oct
      TL1      D8: tilt is -g/2 below, +g/2 above, 0 dB at the pivot
      Z1       0 dB is the identity
      P1       D10: bell re-centre continuity; notch / band-pass clamps
      NB1-NB4  notch / band-pass regression ceilings below their clamps
      A1       no allocation in design() (malloc_logger)
      B1       two threads, same bits
      INFO     printed, never failing

  ==============================================================================
*/

#include "dsp/EqTypes.h"
#include "dsp/ResponseEval.h"
#include "dsp/Prototype.h"
#include "dsp/Designer.h"
#include "dsp/Polish.h"
#include "dsp/PolishConstants.h"
#include "dsp/BandParams.h"
#include "dsp/SectionEngine.h"
#include "dsp/Band.h"
#include "dsp/Router.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <complex>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <limits>
#include <string>
#include <thread>
#include <vector>

#if defined (__APPLE__)
 #include <pthread.h>

 // libmalloc's stack-logging hook (what MallocStackLogging / Instruments attach to).
 extern "C"
 {
     typedef void (malloc_logger_t) (uint32_t type, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
                                     uintptr_t result, uint32_t numHotFramesToSkip);
     extern malloc_logger_t* malloc_logger;
 }
#endif

#if defined (__clang__) || defined (__GNUC__)
 #pragma GCC diagnostic ignored "-Wfloat-equal"
#endif

#ifndef GOLDEN_DIR
 #error "GOLDEN_DIR must point at tests/golden"
#endif

namespace
{
    //==========================================================================
    // regression ceilings: set from the first measured run + 15 %
    //
    // Rows CS1 / CS2: cut accuracy against the analog prototype at 44.1 kHz,
    // Q 1, error counted where the prototype is above -60 dB, 20 Hz - 20 kHz.
    // CS1 starts from VERIFICATION-2.1 issue 4 (1.35 / 0.82 / 0.01 dB measured
    // there with another scorer). CS2 holds placeholders until the first run.
    //==========================================================================
    constexpr double kCs1LowCut12Ceiling  = 1.55;       // dB, Low Cut 12 dB/oct, fc 2 - 20 kHz
    constexpr double kCs1LowCut24Ceiling  = 0.95;       // dB, Low Cut 24 dB/oct, fc 2 - 20 kHz
    constexpr double kCs1LowCut96Ceiling  = 0.05;       // dB, Low Cut 96 dB/oct, fc 2 - 20 kHz
    constexpr double kCs2HighCut12Ceiling = 1.27;       // dB, High Cut 12 dB/oct, fc <= 10 kHz   (measured 1.0966)
    constexpr double kCs2HighCut48Ceiling = 4.58;       // dB, High Cut 48 dB/oct, fc <= 10 kHz   (measured 3.9805)
    constexpr double kCs2HighCut96Ceiling = 4.85;       // dB, High Cut 96 dB/oct, fc <= 10 kHz   (measured 4.2161)
    //==========================================================================

    constexpr double kPi = eq::pi;
    constexpr double kTiny = 1.0e-300;
    constexpr double kInf = std::numeric_limits<double>::infinity();

    constexpr std::array<double, 5> kRates5 { { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 } };
    constexpr std::array<double, 8> kRates8 { { 22050.0, 32000.0, 44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0 } };
    constexpr std::array<double, 5> kG1Gains { { 3.0, 6.0, 12.0, 18.0, 30.0 } };

    const char* const kTypeNames[8]  = { "bell", "lowShelf", "highShelf", "lowCut", "highCut", "notch", "bandPass", "tilt" };
    const char* const kClassNames[8] = { "gain2", "gain1", "cutLp2", "cutHp2", "cutLp1", "cutHp1", "notch", "bandPass" };

    //==========================================================================
    // Rows
    //==========================================================================
    int passes = 0, failures = 0, infos = 0;

   #if defined (__GNUC__)
    __attribute__ ((format (printf, 1, 2)))
   #endif
    std::string strf (const char* format, ...)
    {
        char text[640];
        va_list args;
        va_start (args, format);
        std::vsnprintf (text, sizeof (text), format, args);
        va_end (args);
        return text;
    }

    void row (bool ok, const std::string& name, const std::string& detail = {})
    {
        std::printf ("  [%s] %-66s %s\n", ok ? "PASS" : "FAIL", name.c_str(), detail.c_str());
        std::fflush (stdout);

        if (ok) ++passes; else ++failures;
    }

    void info (const std::string& name, const std::string& detail = {})
    {
        std::printf ("  [INFO] %-66s %s\n", name.c_str(), detail.c_str());
        std::fflush (stdout);
        ++infos;
    }

    void note (const std::string& text)
    {
        std::printf ("           %s\n", text.c_str());
    }

    void heading (const char* title)
    {
        std::printf ("\n-- %s\n", title);
        std::fflush (stdout);
    }

    //==========================================================================
    // Small helpers
    //==========================================================================
    std::size_t ix (int i)
    {
        return static_cast<std::size_t> (i);
    }

    eq::BandSettings mk (eq::Type type, double fc, double gainDb, double q, int slopeIndex)
    {
        eq::BandSettings s;
        s.type = type;
        s.freqHz = fc;
        s.gainDbEff = gainDb;
        s.q = q;
        s.slopeIndex = slopeIndex;
        return s;
    }

    /** The closed form (Phase 2.1) or the polished design (Phase 2.1b). */
    void designBand (const eq::Designer& designer, bool polished, const eq::BandSettings& s, eq::BandDesign& out)
    {
        if (polished)
            designer.design (s, out);
        else
            designer.designSeed (s, out);
    }

    /** The SEED design: what every Phase 2.1 row scores. */
    eq::BandDesign designAt (double fs, const eq::BandSettings& s)
    {
        eq::Designer designer;
        designer.prepare (fs);
        eq::BandDesign d {};
        designer.designSeed (s, d);
        return d;
    }

    std::string describeSettings (const eq::BandSettings& s)
    {
        char text[200];
        std::snprintf (text, sizeof (text), "%s slope-index %d fc %.9g q %.9g gain %.9g",
                       s.type == eq::Type::bell ? "bell"
                         : (s.type == eq::Type::lowShelf ? "lowShelf"
                         : (s.type == eq::Type::highShelf ? "highShelf"
                         : (s.type == eq::Type::tilt ? "tilt" : "other"))),
                       s.slopeIndex, s.freqHz, s.q, s.gainDbEff);
        return text;
    }

    /** Runs fn (0) ... fn (count - 1) on worker threads. Callers store results
        per index, so the outcome does not depend on thread count or scheduling. */
    template <typename Fn>
    void parallelFor (int count, Fn&& fn)
    {
        if (count <= 0)
            return;

        const unsigned hardware = std::thread::hardware_concurrency();
        const int workers = std::max (1, std::min (count, static_cast<int> (hardware == 0 ? 4u : hardware)));
        std::atomic<int> next { 0 };
        std::vector<std::thread> pool;
        pool.reserve (static_cast<std::size_t> (workers));

        for (int wk = 0; wk < workers; ++wk)
            pool.emplace_back ([&next, &fn, count]
            {
                for (;;)
                {
                    const int index = next.fetch_add (1);

                    if (index >= count)
                        return;

                    fn (index);
                }
            });

        for (std::thread& worker : pool)
            worker.join();
    }

    std::vector<double> geomspace (double lo, double hi, int n)
    {
        std::vector<double> v;
        v.reserve (ix (n));

        for (int i = 0; i < n; ++i)
            v.push_back (n > 1 ? lo * std::pow (hi / lo, static_cast<double> (i) / static_cast<double> (n - 1)) : lo);

        return v;
    }

    bool sameBits (double a, double b)
    {
        return std::memcmp (&a, &b, sizeof (double)) == 0;
    }

    /** Bitwise comparison of the USED parameters, the classes, the flags and
        the counts. withProto adds the true analog sections. */
    bool sameDesign (const eq::BandDesign& a, const eq::BandDesign& b, bool withProto)
    {
        if (a.numSections != b.numSections || a.inverted != b.inverted)
            return false;

        for (int i = 0; i < a.numSections; ++i)
        {
            const eq::SectionParams& p = a.params[ix (i)];
            const eq::SectionParams& q = b.params[ix (i)];

            if (a.cls[ix (i)] != b.cls[ix (i)] || p.hasSvf != q.hasSvf || p.hasOnePole != q.hasOnePole)
                return false;

            if (p.hasSvf && ! (sameBits (p.g, q.g) && sameBits (p.k, q.k) && sameBits (p.mL, q.mL)
                                && sameBits (p.mB, q.mB) && sameBits (p.mH, q.mH)))
                return false;

            if (p.hasOnePole && ! (sameBits (p.G, q.G) && sameBits (p.cL, q.cL) && sameBits (p.cH, q.cH)))
                return false;

            if (std::memcmp (&a.polishWeight[ix (i)], &b.polishWeight[ix (i)], sizeof (float)) != 0)
                return false;

            if (withProto && std::memcmp (&a.trueProto[ix (i)], &b.trueProto[ix (i)], sizeof (eq::AnalogSection)) != 0)
                return false;
        }

        return true;
    }

    /** Independent of eq::sectionCount on purpose. */
    int expectedSections (eq::Type type, int slopeIndex)
    {
        static const int orders[8] = { 1, 2, 3, 4, 6, 8, 12, 16 };

        switch (type)
        {
            case eq::Type::bell:
            case eq::Type::notch:
            case eq::Type::bandPass:
                return 1;

            case eq::Type::lowShelf:
            case eq::Type::highShelf:
            case eq::Type::lowCut:
            case eq::Type::highCut:
            case eq::Type::tilt:
                break;
        }

        const int order = orders[slopeIndex];
        return order / 2 + order % 2;
    }

    //==========================================================================
    // Seeded random settings (own generator: the same stream on every platform)
    //==========================================================================
    struct Rng
    {
        explicit Rng (std::uint64_t seed) : state (seed) {}

        std::uint64_t next()
        {
            state += 0x9E3779B97F4A7C15ull;
            std::uint64_t z = state;
            z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
            z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
            return z ^ (z >> 31);
        }

        double uniform()
        {
            return static_cast<double> (next() >> 11) * (1.0 / 9007199254740992.0);
        }

        int below (int n)
        {
            return static_cast<int> (next() % static_cast<std::uint64_t> (n));
        }

        std::uint64_t state;
    };

    /** Type and slope uniform; fc 10 Hz - 30 kHz log; Q 0.025 - 40 log; gain +-60 dB. */
    eq::BandSettings randomSettings (Rng& rng)
    {
        const int type = rng.below (8);
        const int slope = rng.below (8);
        const double fc = 10.0 * std::pow (3000.0, rng.uniform());
        const double q = 0.025 * std::pow (1600.0, rng.uniform());
        const double gain = -60.0 + 120.0 * rng.uniform();
        return mk (static_cast<eq::Type> (type), fc, gain, q, slope);
    }

    //==========================================================================
    // Evaluation grid (P6): log points plus points tied to each section's
    // zero pair and pole pair
    //==========================================================================
    struct Grid
    {
        double fs = 0.0, wLo = 0.0, wHi = 0.0;
        std::vector<double> w, t2;
    };

    Grid makeGrid (double fs, int n, double fLo, double fHiRequested, double nyquistFraction)
    {
        Grid grid;
        const double fHi = std::min (fHiRequested, nyquistFraction * fs);
        grid.fs = fs;
        grid.wLo = 2.0 * kPi * fLo / fs;
        grid.wHi = 2.0 * kPi * fHi / fs;

        for (const double f : geomspace (fLo, fHi, n))
        {
            const double w = 2.0 * kPi * f / fs;
            grid.w.push_back (w);
            grid.t2.push_back (eq::tanSquaredHalf (w));
        }

        return grid;
    }

    struct Tied
    {
        std::array<double, 8 * 34> w {};
        std::array<double, 8 * 34> t2 {};
        int n = 0;
    };

    void addTiedPoint (Tied& tied, double w, double lo, double hi)
    {
        if (tied.n >= static_cast<int> (tied.w.size()) || std::isnan (w))
            return;

        const double c = std::min (std::max (w, lo), hi);
        tied.w[ix (tied.n)] = c;
        tied.t2[ix (tied.n)] = eq::tanSquaredHalf (c);
        ++tied.n;
    }

    /** The centre and w0 (sqrt (1 + h^2) +- h), h = s / (2Q), eight values of s. */
    void addTiedPair (Tied& tied, double w0, double Q, double lo, double hi)
    {
        static const double scales[8] = { 0.1, 0.25, 0.5, 0.75, 1.0, 1.5, 2.0, 3.0 };

        addTiedPoint (tied, w0, lo, hi);

        for (const double s : scales)
        {
            const double h = s / (2.0 * Q);
            const double rt = std::sqrt (1.0 + h * h);
            addTiedPoint (tied, w0 * (rt + h), lo, hi);
            addTiedPoint (tied, w0 / (rt + h), lo, hi);      // = w0 (rt - h), without the cancellation
        }
    }

    void collectTied (const eq::BandDesign& analog, double lo, double hi, Tied& tied)
    {
        tied.n = 0;

        for (int i = 0; i < analog.numSections; ++i)
        {
            const eq::AnalogSection& a = analog.trueProto[ix (i)];

            switch (analog.cls[ix (i)])
            {
                case eq::SectionClass::gain2:
                    addTiedPair (tied, a.wz, a.Qz, lo, hi);
                    addTiedPair (tied, a.wp, a.Qp, lo, hi);
                    break;

                case eq::SectionClass::gain1:
                    addTiedPoint (tied, a.wz, lo, hi);
                    addTiedPoint (tied, a.wp, lo, hi);
                    break;

                case eq::SectionClass::cutLp2:
                case eq::SectionClass::cutHp2:
                case eq::SectionClass::notch:
                case eq::SectionClass::bandPass:
                    addTiedPair (tied, a.wp, a.Qp, lo, hi);
                    break;

                case eq::SectionClass::cutLp1:
                case eq::SectionClass::cutHp1:
                    addTiedPoint (tied, a.wp, lo, hi);
                    break;
            }
        }
    }

    double bandDigitalMag2 (const eq::BandDesign& d, double t2)
    {
        double m = 1.0;

        for (int i = 0; i < d.numSections; ++i)
            m *= eq::sectionMag2 (t2, d.params[ix (i)]);

        return m;
    }

    /** TRUE analog prototype power; the reciprocal when the band is a cut. */
    double bandAnalogMag2 (const eq::BandDesign& d, double w)
    {
        double m = 1.0;

        for (int i = 0; i < d.numSections; ++i)
            m *= eq::analogMag2 (d.trueProto[ix (i)], d.cls[ix (i)], w);

        return d.inverted ? 1.0 / m : m;
    }

    /** Largest |digital - analog| in dB over the grid (plus the tied points of
        `analog` when asked), counting only points where the analog prototype is
        above floorDb. `digital` supplies the section parameters, `analog` the
        true prototype; they are the same object except for the RBJ control.
        Any non-finite value returns +infinity. */
    double maxError (const eq::BandDesign& digital, const eq::BandDesign& analog, const Grid& grid,
                     bool withTied, double floorDb = -1.0e9)
    {
        const double floorPower = floorDb < -2000.0 ? 0.0 : std::pow (10.0, floorDb / 10.0);
        double worst = 0.0;
        bool bad = false;

        const auto point = [&] (double w, double t2)
        {
            const double a = bandAnalogMag2 (analog, w);

            if (std::isnan (a))
            {
                bad = true;
                return;
            }

            if (! (a > floorPower))
                return;

            const double d = bandDigitalMag2 (digital, t2);
            const double e = std::fabs (10.0 * std::log10 ((d < kTiny ? kTiny : d) / a));

            if (! std::isfinite (e))
                bad = true;
            else if (e > worst)
                worst = e;
        };

        for (std::size_t i = 0; i < grid.w.size(); ++i)
            point (grid.w[i], grid.t2[i]);

        if (withTied)
        {
            Tied tied;
            collectTied (analog, grid.wLo, grid.wHi, tied);

            for (int i = 0; i < tied.n; ++i)
                point (tied.w[ix (i)], tied.t2[ix (i)]);
        }

        return bad ? kInf : worst;
    }

    /** Worst maxError over types x gains x fcs x qs at one rate. The seed
        design unless `polished`. */
    double zoneWorst (double fs, const std::vector<eq::Type>& types, int slopeIndex,
                      const std::vector<double>& fcs, const std::vector<double>& qs,
                      std::initializer_list<double> gains, const Grid& grid, bool withTied,
                      double floorDb = -1.0e9, bool polished = false)
    {
        eq::Designer designer;
        designer.prepare (fs);
        eq::BandDesign d {};
        double worst = 0.0;

        for (const eq::Type type : types)
            for (const double gain : gains)
                for (const double fc : fcs)
                    for (const double q : qs)
                    {
                        designBand (designer, polished, mk (type, fc, gain, q, slopeIndex), d);
                        worst = std::max (worst, maxError (d, d, grid, withTied, floorDb));
                    }

        return worst;
    }

    //==========================================================================
    // D1: the golden
    //==========================================================================
    struct GoldenSetting
    {
        double fs;
        int type;
        int slope;
        double fc, q, gain;
    };

    /** The settings list of tests/model/dump_designs.py, in its loop order. */
    std::vector<GoldenSetting> goldenSettings()
    {
        static const int slopes[6] = { 0, 1, 2, 3, 5, 7 };
        static const double fcs[11] = { 10.0, 20.0, 100.0, 999.99988, 5000.0, 12000.0, 18000.0, 20900.0, 21500.0, 25000.0, 30000.0 };
        static const double qs[7] = { 0.025, 0.3, 0.70710678, 0.99999976, 1.41, 4.0, 40.0 };
        static const double gains[7] = { -60.0, -30.0, -6.0, 0.0, 3.0, 18.0, 60.0 };

        std::vector<GoldenSetting> list;

        for (const double fs : kRates5)
            for (int type = 0; type < 8; ++type)
            {
                const auto t = static_cast<eq::Type> (type);
                const bool slopeInert = ! eq::usesSlope (t);
                const bool gainBearing = eq::isGainType (t);

                for (const int si : slopes)
                {
                    if (slopeInert && si != 1)
                        continue;

                    for (const double fc : fcs)
                        for (const double q : qs)
                        {
                            if (gainBearing)
                            {
                                for (const double gain : gains)
                                    list.push_back (GoldenSetting { fs, type, si, fc, q, gain });
                            }
                            else
                            {
                                list.push_back (GoldenSetting { fs, type, si, fc, q, 0.0 });
                            }
                        }
                }
            }

        return list;
    }

    /** The settings list of tests/model/dump_polish.py: the list above
        restricted to 44.1 / 48 / 96 kHz and the four gain types, same order. */
    std::vector<GoldenSetting> polishedGoldenSettings()
    {
        std::vector<GoldenSetting> list;

        for (const GoldenSetting& gs : goldenSettings())
        {
            const bool rate = sameBits (gs.fs, 44100.0) || sameBits (gs.fs, 48000.0) || sameBits (gs.fs, 96000.0);

            if (rate && eq::isGainType (static_cast<eq::Type> (gs.type)))
                list.push_back (gs);
        }

        return list;
    }

    void appendNumber (std::string& out, double x)
    {
        if (! std::isfinite (x))
        {
            std::fprintf (stderr, "designer-harness: non-finite value in --dump-designs / --dump-polished\n");
            std::exit (2);
        }

        char text[48];
        std::snprintf (text, sizeof (text), "%.17g", x + 0.0);      // + 0.0 turns a negative zero into 0
        out += text;
    }

    /** stdout only, "\n" endings only, the structure of dump_designs.py
        (seed) or of dump_polish.py (polished: its settings list and a final
        weight column). */
    int dumpDesigns (bool polished)
    {
        if (polished)
        {
            std::fputs ("# GENERATED by tests/model/dump_polish.py; do not edit\n", stdout);
            std::fputs ("# fs\ttype\tslope\tfc\tq\tgain\tsec\tcls\thasSvf\thasOnePole\tg\tk\tmL\tmB\tmH\tG\tcL\tcH\tweight\n", stdout);
        }
        else
        {
            std::fputs ("# GENERATED by tests/model/dump_designs.py; do not edit\n", stdout);
            std::fputs ("# fs\ttype\tslope\tfc\tq\tgain\tsec\tcls\thasSvf\thasOnePole\tg\tk\tmL\tmB\tmH\tG\tcL\tcH\n", stdout);
        }

        eq::Designer designer;
        eq::BandDesign d {};
        std::string head, line;
        const std::vector<GoldenSetting> dumpList = polished ? polishedGoldenSettings() : goldenSettings();

        for (const GoldenSetting& gs : dumpList)
        {
            designer.prepare (gs.fs);
            designBand (designer, polished, mk (static_cast<eq::Type> (gs.type), gs.fc, gs.gain, gs.q, gs.slope), d);

            head.clear();
            appendNumber (head, gs.fs);
            head += '\t';
            head += kTypeNames[gs.type];
            head += '\t';
            head += std::to_string (gs.slope);
            head += '\t';
            appendNumber (head, gs.fc);
            head += '\t';
            appendNumber (head, gs.q);
            head += '\t';
            appendNumber (head, gs.gain);

            for (int i = 0; i < d.numSections; ++i)
            {
                const eq::SectionParams& p = d.params[ix (i)];
                const double values[8] = { p.g, p.k, p.mL, p.mB, p.mH, p.G, p.cL, p.cH };

                line = head;
                line += '\t';
                line += std::to_string (i);
                line += '\t';
                line += kClassNames[static_cast<std::size_t> (d.cls[ix (i)])];
                line += p.hasSvf ? "\t1" : "\t0";
                line += p.hasOnePole ? "\t1" : "\t0";

                for (int c = 0; c < 8; ++c)
                {
                    line += '\t';

                    if (c < 5 ? p.hasSvf : p.hasOnePole)
                        appendNumber (line, values[c]);
                    else
                        line += '-';
                }

                if (polished)
                {
                    line += '\t';
                    appendNumber (line, static_cast<double> (d.polishWeight[ix (i)]));
                }

                line += '\n';
                std::fwrite (line.data(), 1, line.size(), stdout);
            }
        }

        std::fflush (stdout);
        return 0;
    }

    bool parseDouble (const std::string& text, double& value)
    {
        if (text.empty())
            return false;

        char* end = nullptr;
        value = std::strtod (text.c_str(), &end);
        return end == text.c_str() + text.size();
    }

    void splitTabs (const std::string& line, std::vector<std::string>& cells)
    {
        cells.clear();
        std::size_t start = 0;

        for (;;)
        {
            const std::size_t tab = line.find ('\t', start);

            if (tab == std::string::npos)
            {
                cells.push_back (line.substr (start));
                return;
            }

            cells.push_back (line.substr (start, tab - start));
            start = tab + 1;
        }
    }

    void rowD1 (const std::string& goldenDir)
    {
        const char* const name = "D1 C++ designs == golden designs.tsv (1e-9 rel; classes, flags, line count)";
        const std::string path = goldenDir + "/designs.tsv";
        std::ifstream in (path, std::ios::binary);

        if (! in.is_open())
        {
            row (false, name, "cannot read " + path);
            return;
        }

        const std::vector<GoldenSetting> settings = goldenSettings();
        long long expectedLines = 0;

        for (const GoldenSetting& gs : settings)
            expectedLines += expectedSections (static_cast<eq::Type> (gs.type), gs.slope);

        eq::Designer designer;
        eq::BandDesign current {};
        std::size_t settingIndex = 0;
        int sec = 0;
        bool haveBand = false, aligned = true;
        long long lineNumber = 0, dataLines = 0, cellsCompared = 0, cellsBad = 0, shapeBad = 0, bitExact = 0;
        double worstRel = 0.0;
        std::string firstProblem;
        std::string line;
        std::vector<std::string> cells;

        while (std::getline (in, line))
        {
            ++lineNumber;

            if (line.empty() || line[0] == '#')
                continue;

            ++dataLines;

            if (! aligned)
                continue;

            if (! haveBand)
            {
                if (settingIndex >= settings.size())
                {
                    aligned = false;
                    firstProblem = strf ("line %lld: more data lines than the settings list produces", lineNumber);
                    continue;
                }

                const GoldenSetting& next = settings[settingIndex];
                designer.prepare (next.fs);
                designer.designSeed (mk (static_cast<eq::Type> (next.type), next.fc, next.gain, next.q, next.slope), current);
                sec = 0;
                haveBand = true;
            }

            const GoldenSetting& gs = settings[settingIndex];
            splitTabs (line, cells);

            double fsCell = 0.0, fcCell = 0.0, qCell = 0.0, gainCell = 0.0;
            const bool headOk = cells.size() == 18
                             && parseDouble (cells[0], fsCell) && fsCell == gs.fs
                             && cells[1] == kTypeNames[gs.type]
                             && cells[2] == std::to_string (gs.slope)
                             && parseDouble (cells[3], fcCell) && fcCell == gs.fc
                             && parseDouble (cells[4], qCell) && qCell == gs.q
                             && parseDouble (cells[5], gainCell) && gainCell == gs.gain
                             && cells[6] == std::to_string (sec)
                             && sec < current.numSections;

            if (! headOk)
            {
                aligned = false;
                firstProblem = strf ("line %lld: expected %s fs %g slope %d fc %g q %g gain %g section %d of %d",
                                     lineNumber, kTypeNames[gs.type], gs.fs, gs.slope, gs.fc, gs.q, gs.gain,
                                     sec, current.numSections);
                continue;
            }

            const eq::SectionParams& p = current.params[ix (sec)];
            const double values[8] = { p.g, p.k, p.mL, p.mB, p.mH, p.G, p.cL, p.cH };
            bool shapeOk = cells[7] == kClassNames[static_cast<std::size_t> (current.cls[ix (sec)])]
                        && cells[8] == (p.hasSvf ? "1" : "0")
                        && cells[9] == (p.hasOnePole ? "1" : "0");

            for (int c = 0; c < 8; ++c)
            {
                const std::string& cell = cells[ix (10 + c)];
                const bool used = c < 5 ? p.hasSvf : p.hasOnePole;
                double gold = 0.0;

                if (! used)
                {
                    shapeOk = shapeOk && cell == "-";
                    continue;
                }

                if (! parseDouble (cell, gold))
                {
                    shapeOk = false;
                    continue;
                }

                const double mine = values[c];
                ++cellsCompared;

                if (sameBits (mine + 0.0, gold))
                    ++bitExact;

                bool ok = std::isfinite (mine);

                if (ok && gold == 0.0)
                {
                    ok = std::fabs (mine) <= 1.0e-12;
                }
                else if (ok)
                {
                    const double rel = std::fabs (mine - gold) / std::fabs (gold);
                    worstRel = std::max (worstRel, rel);
                    ok = rel <= 1.0e-9;
                }

                if (! ok)
                {
                    ++cellsBad;

                    if (firstProblem.empty())
                        firstProblem = strf ("line %lld column %d: C++ %.17g, golden %.17g", lineNumber, 11 + c, mine, gold);
                }
            }

            if (! shapeOk)
            {
                ++shapeBad;

                if (firstProblem.empty())
                    firstProblem = strf ("line %lld: class / flag / unused-cell mismatch", lineNumber);
            }

            if (++sec == current.numSections)
            {
                haveBand = false;
                ++settingIndex;
            }
        }

        if (aligned && (haveBand || settingIndex != settings.size()) && firstProblem.empty())
            firstProblem = strf ("golden ends early: %zu of %zu settings seen", settingIndex, settings.size());

        const bool ok = aligned && ! haveBand && settingIndex == settings.size()
                     && dataLines == expectedLines && cellsBad == 0 && shapeBad == 0 && cellsCompared > 0;

        row (ok, name,
             strf ("%lld data lines (C++ list: %lld), %lld cells, %lld out of tolerance, %lld shape, worst rel %.3g, %lld bit-exact",
                   dataLines, expectedLines, cellsCompared, cellsBad, shapeBad, worstRel, bitExact)
                 + (firstProblem.empty() ? std::string() : "  first: " + firstProblem));
    }

    //==========================================================================
    // E1: closed-form response vs the recurrences
    //==========================================================================
    struct FixedDesign
    {
        eq::Type type;
        double fc, gain, q;
        int slope;
    };

    void rowE1()
    {
        using T = eq::Type;

        // 40 fixed designs, fc >= 200 Hz, Q <= 10, every section class.
        static const FixedDesign designs[40] =
        {
            { T::bell, 200.0, 6.0, 0.5, 1 },        { T::bell, 1000.0, -18.0, 2.0, 1 },
            { T::bell, 8000.0, 18.0, 10.0, 1 },     { T::bell, 18000.0, -6.0, 1.0, 1 },
            { T::bell, 999.99988, 12.0, 0.99999976, 1 },

            { T::lowShelf, 250.0, 9.0, 1.0, 0 },    { T::lowShelf, 1000.0, -12.0, 0.7, 1 },
            { T::lowShelf, 4000.0, 6.0, 1.0, 2 },   { T::lowShelf, 500.0, 18.0, 1.0, 7 },

            { T::highShelf, 3000.0, -9.0, 1.0, 0 }, { T::highShelf, 8000.0, 12.0, 1.4, 1 },
            { T::highShelf, 200.0, 18.0, 0.5, 3 },  { T::highShelf, 15000.0, -6.0, 1.0, 5 },
            { T::highShelf, 1000.0, 6.0, 3.0, 6 },

            { T::tilt, 1000.0, 6.0, 1.0, 0 },       { T::tilt, 700.0, -12.0, 1.0, 1 },
            { T::tilt, 5000.0, 18.0, 0.8, 2 },      { T::tilt, 2000.0, -6.0, 1.0, 7 },

            { T::lowCut, 200.0, 0.0, 1.0, 0 },      { T::lowCut, 400.0, 0.0, 1.0, 1 },
            { T::lowCut, 1000.0, 0.0, 2.0, 2 },     { T::lowCut, 2500.0, 0.0, 0.5, 3 },
            { T::lowCut, 300.0, 0.0, 1.0, 4 },      { T::lowCut, 5000.0, 0.0, 4.0, 5 },
            { T::lowCut, 800.0, 0.0, 1.0, 6 },      { T::lowCut, 12000.0, 0.0, 1.0, 7 },

            { T::highCut, 18000.0, 0.0, 1.0, 0 },   { T::highCut, 9000.0, 0.0, 0.7, 1 },
            { T::highCut, 4000.0, 0.0, 1.0, 2 },    { T::highCut, 1500.0, 0.0, 3.0, 3 },
            { T::highCut, 600.0, 0.0, 1.0, 4 },     { T::highCut, 12000.0, 0.0, 10.0, 5 },
            { T::highCut, 3000.0, 0.0, 1.0, 6 },    { T::highCut, 200.0, 0.0, 1.0, 7 },

            { T::notch, 200.0, 0.0, 10.0, 1 },      { T::notch, 3000.0, 0.0, 1.0, 1 },
            { T::notch, 16000.0, 0.0, 0.3, 1 },

            { T::bandPass, 200.0, 0.0, 10.0, 1 },   { T::bandPass, 2500.0, 0.0, 0.7, 1 },
            { T::bandPass, 18000.0, 0.0, 2.0, 1 }
        };

        const double fs = 48000.0;
        const int numSamples = 1 << 17;
        const int numFreqs = 64;
        std::vector<double> h (ix (numSamples));
        bool classSeen[8] = {};
        double worstDb = 0.0, worstRel = 0.0;
        long long compared = 0;
        bool bad = false;

        for (const FixedDesign& f : designs)
        {
            const eq::BandDesign d = designAt (fs, mk (f.type, f.fc, f.gain, f.q, f.slope));

            // ARCHITECTURE "SVF section and one-pole", run sample by sample on a unit impulse.
            double ic1[8] = {}, ic2[8] = {}, st[8] = {};
            double a1[8] = {}, a2[8] = {}, a3[8] = {};

            for (int i = 0; i < d.numSections; ++i)
            {
                const eq::SectionParams& p = d.params[ix (i)];
                classSeen[static_cast<std::size_t> (d.cls[ix (i)])] = true;
                a1[i] = 1.0 / (1.0 + p.g * (p.g + p.k));
                a2[i] = p.g * a1[i];
                a3[i] = p.g * a2[i];
            }

            for (int n = 0; n < numSamples; ++n)
            {
                double x = n == 0 ? 1.0 : 0.0;

                for (int i = 0; i < d.numSections; ++i)
                {
                    const eq::SectionParams& p = d.params[ix (i)];

                    if (p.hasSvf)
                    {
                        const double v3 = x - ic2[i];
                        const double v1 = a1[i] * ic1[i] + a2[i] * v3;
                        const double v2 = ic2[i] + a2[i] * ic1[i] + a3[i] * v3;
                        ic1[i] = 2.0 * v1 - ic1[i];
                        ic2[i] = 2.0 * v2 - ic2[i];
                        x = p.mL * v2 + p.mB * v1 + p.mH * (x - p.k * v1 - v2);
                    }

                    if (p.hasOnePole)
                    {
                        const double v = (x - st[i]) * p.G / (1.0 + p.G);
                        const double lp = v + st[i];
                        st[i] = lp + v;
                        x = p.cL * lp + p.cH * (x - lp);
                    }
                }

                h[ix (n)] = x;
            }

            for (int k = 0; k < numFreqs; ++k)
            {
                const double freq = 50.0 * std::pow (400.0, static_cast<double> (k) / static_cast<double> (numFreqs - 1));
                const double w = 2.0 * kPi * freq / fs;
                const double closedDb = eq::bandMagDb (d, w);

                if (! std::isfinite (closedDb))
                {
                    bad = true;
                    continue;
                }

                if (! (closedDb > -100.0))
                    continue;

                // DFT at w by Horner's rule in z^-1 = e^(-j w), last sample first.
                const double c = std::cos (w);
                const double s = -std::sin (w);
                double re = 0.0, im = 0.0;

                for (int n = numSamples - 1; n >= 0; --n)
                {
                    const double nextRe = re * c - im * s + h[ix (n)];
                    im = re * s + im * c;
                    re = nextRe;
                }

                const double measuredDb = 10.0 * std::log10 (std::max (re * re + im * im, kTiny));
                const std::complex<double> closed = eq::bandResponse (d, w);
                const double rel = std::abs (std::complex<double> (re, im) - closed) / std::abs (closed);

                if (! std::isfinite (measuredDb) || ! std::isfinite (rel))
                    bad = true;

                worstDb = std::max (worstDb, std::fabs (measuredDb - closedDb));
                worstRel = std::max (worstRel, rel);
                ++compared;
            }
        }

        bool allClasses = true;

        for (const bool seen : classSeen)
            allClasses = allClasses && seen;

        row (! bad && allClasses && compared > 1000 && worstDb <= 1.0e-6 && worstRel <= 1.2e-7,
             "E1 ResponseEval == impulse through the recurrences (<= 1e-6 dB above -100 dB)",
             strf ("worst %.3g dB, complex %.3g rel (limit 1.2e-7), %lld points, 40 designs%s%s",
                   worstDb, worstRel, compared, allClasses ? "" : ", a section class is MISSING",
                   bad ? ", NON-FINITE value" : ""));
    }

    //==========================================================================
    // G1
    //==========================================================================
    /** shelf12Wide: the 2.1b 12 dB/oct row (Q points extended to 1.41).
        shelfSteep: one even-order slope from 24 dB/oct up, Q 1.0. */
    enum class G1Kind { bell, shelf12, shelf6, shelf12Wide, shelfSteep };

    /** What is scored: the closed form, the polished design, or the RBJ control. */
    enum class G1Source { seed, polished, rbj };

    /** Which tolerance: Phase 2.1's (Stage 0), or Phase 2.1b's. */
    enum class G1Tolerance { phase21, phase21b };

    double g1Tolerance (G1Kind kind, double fs, double gainAbs)
    {
        if (fs >= 88200.0)
            return 0.10 + 0.015 * gainAbs;

        switch (kind)
        {
            case G1Kind::bell:          return 0.15 + 0.05 * gainAbs;
            case G1Kind::shelf12:
            case G1Kind::shelf12Wide:
            case G1Kind::shelfSteep:    return 0.15 + 0.06 * gainAbs;
            case G1Kind::shelf6:        return 0.10 + 0.02 * gainAbs;       // P9: not tightened
        }

        return 0.0;
    }

    /** PLAN-2.1b P1. The 6 dB/oct shelf keeps g1Tolerance. */
    double g1bTolerance (double fs, double gainAbs)
    {
        return fs >= 88200.0 ? 0.03 + 0.002 * gainAbs : 0.05 + 0.012 * gainAbs;
    }

    /** The Q points of a G1 family. */
    std::vector<double> g1Qs (G1Kind kind)
    {
        std::vector<double> qs;

        switch (kind)
        {
            case G1Kind::bell:
                qs = geomspace (0.025, 40.0, 17);
                break;

            case G1Kind::shelf12:
            case G1Kind::shelf12Wide:
                for (const double q : geomspace (0.025, 40.0, 17))
                    if (q <= 1.0 + 1.0e-9)
                        qs.push_back (q);

                qs.push_back (0.70710678);
                qs.push_back (1.0);

                if (kind == G1Kind::shelf12Wide)
                {
                    qs.push_back (1.19);
                    qs.push_back (1.41);
                }

                break;

            case G1Kind::shelf6:            // not read at 6 dB/oct
            case G1Kind::shelfSteep:
                qs.push_back (1.0);
                break;
        }

        return qs;
    }

    /** The control: an RBJ pre-warped bilinear bell, converted to SVF form by
        mL = (b0+b1+b2)/P0, mH = (b0-b1+b2)/P1, mB = 2 (b0-b2)/sqrt (P0 P1),
        with g, k from its denominator. No corrector. */
    eq::SectionParams rbjBell (double fs, double fc, double gainDb, double q)
    {
        const double w0 = 2.0 * kPi * fc / fs;
        const double A = std::pow (10.0, gainDb / 40.0);
        const double alpha = std::sin (w0) / (2.0 * q);
        const double cw = std::cos (w0);
        const double a0 = 1.0 + alpha / A;
        const double b0 = (1.0 + alpha * A) / a0;
        const double b1 = -2.0 * cw / a0;
        const double b2 = (1.0 - alpha * A) / a0;
        const double a1 = -2.0 * cw / a0;
        const double a2 = (1.0 - alpha / A) / a0;
        const double P0 = 1.0 + a1 + a2;
        const double P1 = 1.0 - a1 + a2;
        const double root = std::sqrt (P0 * P1);

        eq::SectionParams p {};
        p.g = std::sqrt (P0 / P1);
        p.k = 2.0 * (1.0 - a2) / root;
        p.mL = (b0 + b1 + b2) / P0;
        p.mB = 2.0 * (b0 - b2) / root;
        p.mH = (b0 - b1 + b2) / P1;
        p.G = 1.0;
        p.cL = 1.0;
        p.cH = 1.0;
        p.hasSvf = true;
        p.hasOnePole = false;
        return p;
    }

    /** Scores one G1 family at the given rates. Returns pass; fills one text
        line per rate (worst / tolerance per gain) and the worst +-30 dB error.
        steepSlopeIndex is read by G1Kind::shelfSteep only; linePrefix goes in
        front of every text line. The 6 dB/oct shelf has one tolerance. */
    bool runG1 (G1Kind kind, G1Source source, G1Tolerance tolerance, const std::vector<double>& rates,
                std::vector<std::string>& lines, double& worstAt30,
                int steepSlopeIndex = 3, const std::string& linePrefix = {})
    {
        const std::vector<double> fcs = geomspace (20.0, 20000.0, 40);
        const std::vector<double> qs = g1Qs (kind);
        const int slope = kind == G1Kind::shelf6 ? 0 : (kind == G1Kind::shelfSteep ? steepSlopeIndex : 1);
        const bool polished = source == G1Source::polished;
        const bool newTolerance = tolerance == G1Tolerance::phase21b && kind != G1Kind::shelf6;
        bool pass = true;
        worstAt30 = 0.0;

        for (const double fs : rates)
        {
            const Grid grid = makeGrid (fs, 1000, 20.0, 20000.0, 0.499);
            eq::Designer designer;
            designer.prepare (fs);
            eq::BandDesign analog {}, digital {};
            std::string line = linePrefix + strf ("fs %6.0f  worst/tol:", fs);

            for (const double gainAbs : kG1Gains)
            {
                double worst = 0.0;

                for (const double sign : { 1.0, -1.0 })
                    for (const double fc : fcs)
                        for (const double q : qs)
                        {
                            if (kind == G1Kind::bell)
                            {
                                designBand (designer, polished, mk (eq::Type::bell, fc, sign * gainAbs, q, slope), analog);
                                digital = analog;

                                if (source == G1Source::rbj)
                                    digital.params[0] = rbjBell (fs, fc, sign * gainAbs, q);

                                worst = std::max (worst, maxError (digital, analog, grid, true));
                            }
                            else
                            {
                                for (const eq::Type type : { eq::Type::lowShelf, eq::Type::highShelf })
                                {
                                    designBand (designer, polished, mk (type, fc, sign * gainAbs, q, slope), analog);
                                    worst = std::max (worst, maxError (analog, analog, grid, true));
                                }
                            }
                        }

                const double tol = newTolerance ? g1bTolerance (fs, gainAbs) : g1Tolerance (kind, fs, gainAbs);

                if (! (worst <= tol))
                    pass = false;

                if (gainAbs > 29.0)
                    worstAt30 = std::max (worstAt30, worst);

                line += strf ("  +-%.0f dB %.3f/%.3f", gainAbs, worst, tol);
            }

            lines.push_back (line);
        }

        return pass;
    }

    void rowsG1()
    {
        struct Family { G1Kind kind; const char* name; };
        static const Family families[3] =
        {
            { G1Kind::bell,    "G1-bell bells, Q 0.025-40: 0.15 + 0.05 |g|; >= 88.2 k 0.10 + 0.015 |g|" },
            { G1Kind::shelf12, "G1-sh12 shelves 12 dB/oct, Q <= 1: 0.15 + 0.06 |g|; >= 88.2 k 0.10 + 0.015 |g|" },
            { G1Kind::shelf6,  "G1-sh6 shelves 6 dB/oct: 0.10 + 0.02 |g|; >= 88.2 k 0.10 + 0.015 |g|" }
        };

        for (const Family& family : families)
        {
            std::vector<std::string> lines;
            double worstAt30 = 0.0;
            const bool pass = runG1 (family.kind, G1Source::seed, G1Tolerance::phase21,
                                     { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 }, lines, worstAt30);
            row (pass, family.name, "seed; five rates, fc 20 Hz-20 kHz x 40, +-3/6/12/18/30 dB");

            for (const std::string& text : lines)
                note (text);
        }

        // G1n: the control must FAIL the same scorer.
        std::vector<std::string> lines;
        double worstAt30 = 0.0;
        const bool rbjPass = runG1 (G1Kind::bell, G1Source::rbj, G1Tolerance::phase21, { 44100.0 }, lines, worstAt30);
        row (! rbjPass, "G1n RBJ bilinear bell FAILS the G1-bell tolerance at 44.1 kHz",
             strf ("worst at +-30 dB %.2f dB (model 13-17)", worstAt30));

        for (const std::string& text : lines)
            note (text);
    }

    /** Phase 2.1b: the tightened G1 and its two negative controls. */
    void rowsG1b()
    {
        const std::vector<double> rates (kRates5.begin(), kRates5.end());
        const char* const gridText = "polished; five rates, fc 20 Hz-20 kHz x 40, +-3/6/12/18/30 dB";

        {
            std::vector<std::string> lines;
            double worstAt30 = 0.0;
            const bool pass = runG1 (G1Kind::bell, G1Source::polished, G1Tolerance::phase21b, rates, lines, worstAt30);
            row (pass, "G1b-bell bells, Q 0.025-40: 0.05 + 0.012 |g|; >= 88.2 k 0.03 + 0.002 |g|", gridText);

            for (const std::string& text : lines)
                note (text);

            note ("model, polished at +-6 / +-12 / +-30 dB: 44.1 k 0.043 / 0.081 / 0.193, 48 k 0.016 / 0.026 / 0.073, 88.2 k up <= 0.013");
        }

        {
            std::vector<std::string> lines;
            double worstAt30 = 0.0;
            const bool pass = runG1 (G1Kind::shelf12Wide, G1Source::polished, G1Tolerance::phase21b, rates, lines, worstAt30);
            row (pass, "G1b-sh12 shelves 12 dB/oct, Q <= 1.41, low and high: same tolerance", gridText);

            for (const std::string& text : lines)
                note (text);

            note ("model at 44.1 k, +-6 / +-12 / +-30 dB: 0.04 / 0.12 / 0.19");
        }

        {
            // Harness-local literals: slope index and its dB/oct.
            static const int steepIndex[5] = { 3, 4, 5, 6, 7 };
            static const int steepDbPerOct[5] = { 24, 36, 48, 72, 96 };
            std::vector<std::string> lines;
            std::string failing;
            bool pass = true;

            for (int s = 0; s < 5; ++s)
            {
                double worstAt30 = 0.0;
                const bool slopePass = runG1 (G1Kind::shelfSteep, G1Source::polished, G1Tolerance::phase21b, rates, lines,
                                              worstAt30, steepIndex[s], strf ("%2d dB/oct  ", steepDbPerOct[s]));

                if (! slopePass)
                {
                    pass = false;
                    failing += strf ("%s%d", failing.empty() ? "" : ", ", steepDbPerOct[s]);
                }
            }

            row (pass, "G1b-shN shelves 24 / 36 / 48 / 72 / 96 dB/oct, Q 1.0, low and high: same tolerance",
                 std::string (gridText) + (failing.empty() ? std::string ("; every slope passes")
                                                           : "; FAILING slopes (dB/oct): " + failing));

            for (const std::string& text : lines)
                note (text);

            note ("model at 44.1 k, 24-96 dB/oct, +-6 / +-12 / +-30 dB: 0.06 / 0.12 / 0.23 (36 and 72 dB/oct were not modelled)");
        }

        {
            std::vector<std::string> lines;
            double worstAt30 = 0.0;
            const bool pass = runG1 (G1Kind::shelf6, G1Source::polished, G1Tolerance::phase21b, rates, lines, worstAt30);
            row (pass, "G1b-sh6 shelves 6 dB/oct with design(): unchanged 0.10 + 0.02 |g|; >= 88.2 k 0.10 + 0.015 |g|",
                 "not polished (first-order section); five rates");

            for (const std::string& text : lines)
                note (text);
        }

        {
            std::vector<std::string> lines;
            double worstAt30 = 0.0;
            const bool seedPass = runG1 (G1Kind::bell, G1Source::seed, G1Tolerance::phase21b, { 44100.0 }, lines, worstAt30);
            row (! seedPass, "G1bn-seed the closed-form seed FAILS the G1b-bell tolerance at 44.1 kHz",
                 strf ("worst at +-30 dB %.3f dB (model 0.34 / 0.61 / 0.98 against 0.12 / 0.19 / 0.41)", worstAt30));

            for (const std::string& text : lines)
                note (text);
        }

        {
            std::vector<std::string> lines;
            double worstAt30 = 0.0;
            const bool rbjPass = runG1 (G1Kind::bell, G1Source::rbj, G1Tolerance::phase21b, { 44100.0 }, lines, worstAt30);
            row (! rbjPass, "G1bn-rbj the RBJ bilinear bell FAILS the G1b-bell tolerance at 44.1 kHz",
                 strf ("worst at +-30 dB %.2f dB", worstAt30));

            for (const std::string& text : lines)
                note (text);
        }
    }

    //==========================================================================
    // T1: the "Model results" rows, scored as research scored them
    // (research-2.1/t1_reproduce.py: 3000 log points 10 Hz - min (20 kHz, 0.4999 fs),
    // no tied points)
    //==========================================================================
    struct T1Spec
    {
        const char* name;
        double fs;
        bool shelf;
        int slope;
        double fLo, fHi;
        int qMode;              // 0: 33 log points 0.025-40; 1: Q 1; 2: 12 log points 0.25-1.41
        double expect[3];       // +-6, +-12, +-30 dB (RESEARCH-2.1 Q1, "This session")
    };

    void rowT1()
    {
        static const T1Spec specs[11] =
        {
            { "bell 44.1k fc<=16k",          44100.0, false, 1, 10.0,    16000.0, 0, { 0.32, 0.58, 0.92 } },
            { "bell 44.1k 16-20k",           44100.0, false, 1, 16000.0, 20000.0, 0, { 0.37, 0.66, 1.05 } },
            { "bell 48k",                    48000.0, false, 1, 10.0,    20000.0, 0, { 0.23, 0.40, 0.57 } },
            { "bell 96k",                    96000.0, false, 1, 10.0,    20000.0, 0, { 0.06, 0.10, 0.14 } },
            { "bell 44.1k 20-30k",           44100.0, false, 1, 20000.0, 30000.0, 0, { 0.95, 1.72, 2.70 } },
            { "shelf12 Q1 fc<=10k",          44100.0, true,  1, 10.0,    10000.0, 1, { 0.05, 0.12, 0.60 } },
            { "shelf12 Q1 10-20k",           44100.0, true,  1, 10000.0, 20000.0, 1, { 0.37, 0.69, 1.19 } },
            { "shelf12 Q.25-1.41 10-20k",    44100.0, true,  1, 10000.0, 20000.0, 2, { 0.66, 1.17, 1.49 } },
            { "shelf6 44.1k fc 10-30k",      44100.0, true,  0, 10.0,    30000.0, 1, { 0.17, 0.31, 0.49 } },
            { "shelf48 Q1 10-20k",           44100.0, true,  5, 10000.0, 20000.0, 1, { 0.84, 1.85, 4.33 } },
            { "shelf12 Q1 20-30k",           44100.0, true,  1, 20000.0, 30000.0, 1, { 0.43, 0.78, 1.22 } }
        };

        static const double gains[3] = { 6.0, 12.0, 30.0 };
        const std::vector<double> q33 = geomspace (0.025, 40.0, 33);
        const std::vector<double> q12 = geomspace (0.25, 1.41, 12);
        const std::vector<double> q1 { 1.0 };

        int bad = 0;
        std::vector<std::string> lines;

        for (const T1Spec& spec : specs)
        {
            const Grid grid = makeGrid (spec.fs, 3000, 10.0, 20000.0, 0.4999);
            const bool fromTen = spec.fLo < 10.5;
            const std::vector<double>& qs = spec.qMode == 0 ? q33 : (spec.qMode == 1 ? q1 : q12);
            std::string line = strf ("%-26s", spec.name);

            for (int gi = 0; gi < 3; ++gi)
            {
                double worst = 0.0;

                if (spec.shelf)
                {
                    const std::vector<double> fcs = geomspace (fromTen ? 10.0 : spec.fLo * 1.0001, spec.fHi, 60);
                    worst = zoneWorst (spec.fs, { eq::Type::lowShelf, eq::Type::highShelf }, spec.slope, fcs, qs,
                                       { gains[gi], -gains[gi] }, grid, false);
                }
                else
                {
                    const std::vector<double> fcs = fromTen ? geomspace (10.0, spec.fHi, 60)
                                                            : geomspace (spec.fLo * 1.0001, spec.fHi, 40);
                    worst = zoneWorst (spec.fs, { eq::Type::bell }, spec.slope, fcs, qs, { gains[gi] }, grid, false);
                }

                const bool ok = std::fabs (worst - spec.expect[gi]) <= 0.05;

                if (! ok)
                    ++bad;

                line += strf ("  %.3f (%.2f)%s", worst, spec.expect[gi], ok ? "" : " <-- OFF");
            }

            lines.push_back (line);
        }

        row (bad == 0, "T1 eleven 'Model results' rows reproduced within 0.05 dB",
             strf ("%d of 33 cells off; measured (expected) at +-6 / +-12 / +-30 dB below", bad));

        for (const std::string& text : lines)
            note (text);
    }

    //==========================================================================
    // R1: reciprocity
    //==========================================================================
    double reciprocityWorst (double fs, eq::Type type, int slopeIndex, const std::vector<double>& fcs,
                             const std::vector<double>& qs, const Grid& grid, bool& flagsOk, bool polished)
    {
        eq::Designer designer;
        designer.prepare (fs);
        eq::BandDesign boost {}, cut {};
        Tied tied;
        double worst = 0.0;

        for (const double gainAbs : kG1Gains)
            for (const double fc : fcs)
                for (const double q : qs)
                {
                    designBand (designer, polished, mk (type, fc, gainAbs, q, slopeIndex), boost);
                    designBand (designer, polished, mk (type, fc, -gainAbs, q, slopeIndex), cut);
                    flagsOk = flagsOk && ! boost.inverted && cut.inverted;

                    const auto point = [&] (double t2)
                    {
                        const double product = bandDigitalMag2 (boost, t2) * bandDigitalMag2 (cut, t2);
                        const double e = std::fabs (10.0 * std::log10 (product));

                        if (! (e <= worst))     // also catches a NaN
                            worst = std::isfinite (e) ? e : kInf;
                    };

                    for (const double t2 : grid.t2)
                        point (t2);

                    collectTied (boost, grid.wLo, grid.wHi, tied);

                    for (int i = 0; i < tied.n; ++i)
                        point (tied.t2[ix (i)]);
                }

        return worst;
    }

    /** R1 (seed) and R2 (polished). */
    void rowReciprocity (bool polished)
    {
        const std::vector<double> fcs = geomspace (20.0, 20000.0, 40);
        const std::vector<double> bellQs = geomspace (0.025, 40.0, 17);
        const std::vector<double> shelfQs { 0.3, 1.0, 4.0 };
        double worstBell = 0.0, worstShelf = 0.0;
        bool flagsOk = true;

        for (const double fs : kRates5)
        {
            const Grid grid = makeGrid (fs, 1000, 20.0, 20000.0, 0.499);
            worstBell = std::max (worstBell, reciprocityWorst (fs, eq::Type::bell, 1, fcs, bellQs, grid, flagsOk, polished));

            for (const eq::Type type : { eq::Type::lowShelf, eq::Type::highShelf, eq::Type::tilt })
                for (int si = 0; si < 8; ++si)
                    worstShelf = std::max (worstShelf, reciprocityWorst (fs, type, si, fcs, shelfQs, grid, flagsOk, polished));
        }

        row (flagsOk && worstBell <= 1.0e-9 && worstShelf <= 1.0e-9,
             polished ? "R2 polished: |H(+g) H(-g)| = 1 within 1e-9 dB (bells; shelves and tilt, 8 slopes, Q 0.3 / 1 / 4)"
                      : "R1 |H(+g) H(-g)| = 1 within 1e-9 dB (bells; shelves and tilt, 8 slopes)",
             strf ("bells %.3g dB, shelves / tilt %.3g dB, five rates%s%s", worstBell, worstShelf,
                   flagsOk ? "" : "; inverted flag WRONG", polished ? " (model 1.2e-12)" : ""));
    }

    //==========================================================================
    // N1: the cancellation case
    //==========================================================================
    /** MatchedPair as it must NOT be written: P0 and P1 formed from a1, a2. */
    eq::MatchedPair naiveMatchedPair (double w, double Q)
    {
        const double q = 1.0 / (2.0 * Q);
        const double r = std::exp (-q * w);
        const double th = std::min (w * std::sqrt (std::max (1.0 - q * q, 0.0)), kPi);
        const double a1 = -2.0 * r * std::cos (th);
        const double a2 = r * r;
        const double f = w / kPi;

        eq::MatchedPair m {};
        m.P0 = 1.0 + a1 + a2;
        m.P1 = std::max (1.0 - a1 + a2, 1.0e-12);
        m.Om = 1.0 - a2;
        m.g = std::sqrt (m.P0 / m.P1);
        m.k = 2.0 * m.Om / std::sqrt (m.P0 * m.P1);
        m.hN = f * f / std::sqrt ((1.0 - f * f) * (1.0 - f * f) + f * f / (Q * Q));
        return m;
    }

    void rowN1()
    {
        const double fs = 192000.0;
        const Grid grid = makeGrid (fs, 400, 2.0, 50.0, 0.499);
        const eq::BandDesign d = designAt (fs, mk (eq::Type::bell, 10.0, 30.0, 40.0, 1));
        const double worst = maxError (d, d, grid, true);

        // Limit 1e-9 dB (VERIFICATION-2.1 issue 3): 1e-6 did not separate the expm1
        // form from the naive one printed below.
        row (worst <= 1.0e-9, "N1 10 Hz, Q 40, +30 dB bell at 192 kHz vs prototype, 2-50 Hz",
             strf ("%.3g dB (limit 1e-9)", worst));

        // The same section from a1 / a2 differences (underdamped pairs here).
        const eq::AnalogSection& a = d.trueProto[0];
        const eq::MatchedPair N = naiveMatchedPair (a.wz, a.Qz);
        const eq::MatchedPair D = naiveMatchedPair (a.wp, a.Qp);
        eq::BandDesign naive = d;
        eq::SectionParams& p = naive.params[0];
        p.g = D.g;
        p.k = D.k;
        p.mL = N.P0 / D.P0;
        p.mB = 2.0 * N.Om / std::sqrt (D.P0 * D.P1);
        p.mH = N.P1 / D.P1;
        p.G = N.g * N.g / N.hN;
        p.cL = a.c * D.P0 / N.P0;
        p.cH = a.c * (D.hN * D.P1) / (N.hN * N.P1);

        info ("N1 the same bell with a naive 1 + a1 + a2 MatchedPair", strf ("%.3g dB", maxError (naive, d, grid, true)));
    }

    //==========================================================================
    // F1: finite / sign / count sweep. No response is scored here (plan R4).
    //==========================================================================
    bool checkDesign (const eq::BandDesign& d, int expectedCount)
    {
        if (d.numSections != expectedCount)
            return false;

        for (int i = 0; i < d.numSections; ++i)
        {
            const eq::SectionParams& p = d.params[ix (i)];

            if (! p.hasSvf && ! p.hasOnePole)
                return false;

            if (p.hasSvf)
            {
                if (! (std::isfinite (p.g) && std::isfinite (p.k) && std::isfinite (p.mL)
                        && std::isfinite (p.mB) && std::isfinite (p.mH)))
                    return false;

                if (! (p.g > 0.0 && p.k > 0.0 && p.mL >= 0.0 && p.mB >= 0.0 && p.mH >= 0.0))
                    return false;
            }

            if (p.hasOnePole)
            {
                if (! (std::isfinite (p.G) && std::isfinite (p.cL) && std::isfinite (p.cH)))
                    return false;

                if (! (p.G > 0.0 && p.cL >= 0.0 && p.cH >= 0.0))
                    return false;
            }
        }

        return true;
    }

    void rowF1()
    {
        std::array<eq::Designer, 8> designers;

        for (std::size_t r = 0; r < kRates8.size(); ++r)
            designers[r].prepare (kRates8[r]);

        Rng rng (0x0F1F1F1F1ull);
        eq::BandDesign d {};
        long long designs = 0, bad = 0;
        long long perType[8] = {};
        std::string first;

        const auto check = [&] (const eq::BandSettings& s)
        {
            const int expected = expectedSections (s.type, s.slopeIndex);

            for (std::size_t r = 0; r < kRates8.size(); ++r)
            {
                designers[r].designSeed (s, d);
                ++designs;

                if (! checkDesign (d, expected))
                {
                    ++bad;

                    if (first.empty())
                        first = strf ("%s fs %.0f fc %.9g q %.9g gain %.9g slope %d", kTypeNames[static_cast<int> (s.type)],
                                      kRates8[r], s.freqHz, s.q, s.gainDbEff, s.slopeIndex);
                }
            }
        };

        for (int n = 0; n < 1000000; ++n)
        {
            const eq::BandSettings s = randomSettings (rng);
            ++perType[static_cast<int> (s.type)];
            check (s);
        }

        // Every combination of the range ends.
        for (int type = 0; type < 8; ++type)
            for (int si = 0; si < 8; ++si)
                for (const double fc : { 10.0, 30000.0 })
                    for (const double q : { 0.025, 40.0 })
                        for (const double gain : { -60.0, -0.0, 0.0, 60.0 })
                            check (mk (static_cast<eq::Type> (type), fc, gain, q, si));

        long long fewest = perType[0];

        for (const long long count : perType)
            fewest = std::min (fewest, count);

        row (bad == 0 && fewest >= 1000,
             "F1 1e6 random settings x 8 rates + range ends: finite, g k G > 0, m c >= 0, count",
             strf ("%lld designs, %lld bad, fewest draws per type %lld%s%s", designs, bad, fewest,
                   fewest >= 1000 ? "" : " (VOID: under 1000)", first.empty() ? "" : ("  first: " + first).c_str()));
    }

    /** True when the F1 checker FAILS both corrupted designs (and passes the clean one). */
    bool f1NegativeFires (std::string& detail)
    {
        const eq::BandSettings s = mk (eq::Type::highShelf, 2000.0, 9.0, 1.0, 2);     // one gain2 + one gain1
        const int expected = expectedSections (s.type, s.slopeIndex);
        const eq::BandDesign clean = designAt (48000.0, s);

        eq::BandDesign withNan = clean;
        withNan.params[0].mB = std::numeric_limits<double>::quiet_NaN();

        eq::BandDesign withMinusOne = clean;
        withMinusOne.params[1].G = -1.0;

        const bool cleanOk = checkDesign (clean, expected);
        const bool nanCaught = ! checkDesign (withNan, expected);
        const bool minusCaught = ! checkDesign (withMinusOne, expected);

        detail = strf ("clean %s, NaN %s, -1 %s", cleanOk ? "passes" : "FAILS",
                       nanCaught ? "caught" : "MISSED", minusCaught ? "caught" : "MISSED");
        return cleanOk && nanCaught && minusCaught;
    }

    //==========================================================================
    // G2s: cut slopes and corner gains
    //==========================================================================
    /** Frequency (Hz) where the response crosses targetDb between fA and fB,
        by bisection on log f. False when the target is not bracketed. */
    bool crossing (const eq::BandDesign& d, double fs, double fA, double fB, double targetDb, double& fOut)
    {
        const auto errorAt = [&] (double logF)
        {
            return eq::bandMagDb (d, 2.0 * kPi * std::exp (logF) / fs) - targetDb;
        };

        double la = std::log (fA), lb = std::log (fB);
        double ea = errorAt (la);
        const double eb = errorAt (lb);

        if (! std::isfinite (ea) || ! std::isfinite (eb) || (ea > 0.0) == (eb > 0.0))
            return false;

        for (int i = 0; i < 80; ++i)
        {
            const double lm = 0.5 * (la + lb);
            const double em = errorAt (lm);

            if ((em > 0.0) == (ea > 0.0))
            {
                la = lm;
                ea = em;
            }
            else
            {
                lb = lm;
            }
        }

        fOut = std::exp (0.5 * (la + lb));
        return true;
    }

    void rowG2s()
    {
        const double fs = 48000.0;
        const std::vector<double> fcs = geomspace (100.0, 2000.0, 9);
        const double cornerQ1 = 10.0 * std::log10 (0.5);                    // -3.0103
        const double cornerQ4 = 20.0 * std::log10 (4.0 / std::sqrt (2.0));  // +9.0309

        int lowCutMeasured = 0, highCutMeasured = 0, highCutSkipped = 0, slopeBad = 0;
        int corner1Bad = 0, corner4Bad = 0, corner1Cells = 0, corner4Cells = 0;
        int highCutMeasuredPerSlope[8] = {};
        double worstSlope = 0.0, worstCorner1 = 0.0, worstCorner4 = 0.0;

        // The nominal slope comes from this harness-local literal table, NOT from
        // eq::slopeOrder, the table under test (VERIFICATION-2.1 issue 2). The row's
        // nominal stays 6.02 dB/oct per order: order = (dB/oct label) / 6.
        static const double nominalDbPerOct[8] = { 6.0, 12.0, 18.0, 24.0, 36.0, 48.0, 72.0, 96.0 };

        // G2s-short (VERIFICATION-2.1 issue 1): the High Cut cells the row above cannot
        // measure, on the span from the -20 dB point to 20 kHz, against the prototype.
        int shortMeasured = 0, shortUnmeasurable = 0, shortBad = 0;
        double worstShort = 0.0;
        double worstShortPerSlope[8] = {};
        int shortMeasuredPerSlope[8] = {};

        for (const eq::Type type : { eq::Type::lowCut, eq::Type::highCut })
            for (int si = 0; si < 8; ++si)
            {
                const double nominal = 20.0 * std::log10 (2.0) * (nominalDbPerOct[si] / 6.0);

                for (const double fc : fcs)
                {
                    const eq::BandDesign d = designAt (fs, mk (type, fc, 0.0, 1.0, si));
                    const double wc = 2.0 * kPi * fc / fs;

                    const double e1 = std::fabs (eq::bandMagDb (d, wc) - cornerQ1);
                    worstCorner1 = std::max (worstCorner1, e1);
                    ++corner1Cells;

                    if (! (e1 <= 0.10))
                        ++corner1Bad;

                    if (si >= 1)
                    {
                        const eq::BandDesign d4 = designAt (fs, mk (type, fc, 0.0, 4.0, si));
                        const double e4 = std::fabs (eq::bandMagDb (d4, wc) - cornerQ4);
                        worstCorner4 = std::max (worstCorner4, e4);
                        ++corner4Cells;

                        if (! (e4 <= 0.25))
                            ++corner4Bad;
                    }

                    double f20 = 0.0, f50 = 0.0;
                    const bool measurable = type == eq::Type::lowCut
                                              ? (crossing (d, fs, fc * 1.0e-3, fc, -20.0, f20) && crossing (d, fs, fc * 1.0e-3, fc, -50.0, f50))
                                              : (crossing (d, fs, fc, 20000.0, -20.0, f20) && crossing (d, fs, fc, 20000.0, -50.0, f50));

                    if (! measurable)
                    {
                        if (type == eq::Type::lowCut)
                        {
                            ++slopeBad;             // a low cut always has both points
                            continue;
                        }

                        ++highCutSkipped;

                        const double fEnd = 20000.0;
                        double fStart = 0.0;

                        if (crossing (d, fs, fc, fEnd, -20.0, fStart) && std::log2 (fEnd / fStart) >= 1.0)
                        {
                            const double octaves = std::log2 (fEnd / fStart);
                            const double wA = 2.0 * kPi * fStart / fs;
                            const double wB = 2.0 * kPi * fEnd / fs;
                            const double digitalSlope = (eq::bandMagDb (d, wA) - eq::bandMagDb (d, wB)) / octaves;
                            const double analogSlope = (eq::analogBandMagDb (d, wA) - eq::analogBandMagDb (d, wB)) / octaves;
                            const double eShort = std::fabs (digitalSlope - analogSlope);

                            ++shortMeasured;
                            ++shortMeasuredPerSlope[si];

                            if (! (eShort <= 1.0))
                                ++shortBad;

                            if (! (eShort <= worstShort))
                                worstShort = std::isfinite (eShort) ? eShort : kInf;

                            if (! (eShort <= worstShortPerSlope[si]))
                                worstShortPerSlope[si] = std::isfinite (eShort) ? eShort : kInf;
                        }
                        else
                        {
                            ++shortUnmeasurable;
                        }

                        continue;
                    }

                    const double slope = 30.0 / std::fabs (std::log2 (f50 / f20));
                    const double e = std::fabs (slope - nominal);
                    worstSlope = std::max (worstSlope, e);

                    if (! (e <= 1.0))
                        ++slopeBad;

                    if (type == eq::Type::lowCut)
                    {
                        ++lowCutMeasured;
                    }
                    else
                    {
                        ++highCutMeasured;
                        ++highCutMeasuredPerSlope[si];
                    }
                }
            }

        row (slopeBad == 0 && lowCutMeasured == 72 && highCutMeasured > 0,
             "G2s stop-band slope between -20 and -50 dB within 1.0 dB/oct of 6.02 N",
             strf ("worst %.3f dB/oct; low cut %d/72 cells, high cut %d measured, %d skipped (-50 dB point above 20 kHz)",
                   worstSlope, lowCutMeasured, highCutMeasured, highCutSkipped));
        note (strf ("high cut cells measured per slope index 0..7: %d %d %d %d %d %d %d %d (of 9 each)",
                    highCutMeasuredPerSlope[0], highCutMeasuredPerSlope[1], highCutMeasuredPerSlope[2], highCutMeasuredPerSlope[3],
                    highCutMeasuredPerSlope[4], highCutMeasuredPerSlope[5], highCutMeasuredPerSlope[6], highCutMeasuredPerSlope[7]));
        row (shortBad == 0 && shortMeasured > 0 && shortMeasured + shortUnmeasurable == highCutSkipped,
             "G2s-short skipped High Cut cells: slope -20 dB point -> 20 kHz within 1.0 dB/oct of the prototype's",
             strf ("worst %.3f dB/oct; %d measured, %d unmeasurable (span under one octave) of %d skipped",
                   worstShort, shortMeasured, shortUnmeasurable, highCutSkipped));
        note (strf ("6 dB/oct: worst %.3f on %d cells (verify measured 0.43);  12 dB/oct: worst %.3f on %d cells "
                    "(verify measured 0.64 over all nine 12 dB/oct cells; only the skipped ones are scored here)",
                    worstShortPerSlope[0], shortMeasuredPerSlope[0], worstShortPerSlope[1], shortMeasuredPerSlope[1]));
        row (corner1Bad == 0 && corner1Cells == 144, "G2s corner gain at Q 1: -3.01 +- 0.10 dB (8 slopes, both cuts)",
             strf ("worst %.4f dB, %d cells", worstCorner1, corner1Cells));
        row (corner4Bad == 0 && corner4Cells == 126, "G2s corner gain at Q 4, slopes >= 12 dB/oct: +9.03 +- 0.25 dB",
             strf ("worst %.4f dB, %d cells", worstCorner4, corner4Cells));
    }

    //==========================================================================
    // SL1, Q1, TL1, Z1, P1
    //==========================================================================
    void rowSL1()
    {
        using T = eq::Type;
        static const FixedDesign settings[20] =
        {
            { T::bell, 100.0, 6.0, 1.0, 0 },        { T::bell, 999.99988, -12.0, 0.99999976, 0 },
            { T::bell, 5000.0, 30.0, 40.0, 0 },     { T::bell, 12000.0, -60.0, 0.025, 0 },
            { T::bell, 21500.0, 18.0, 4.0, 0 },     { T::bell, 30000.0, -6.0, 0.3, 0 },
            { T::bell, 10.0, 3.0, 1.41, 0 },        { T::bell, 18000.0, 0.0, 1.0, 0 },

            { T::notch, 50.0, 0.0, 1.0, 0 },        { T::notch, 999.99988, 0.0, 0.99999976, 0 },
            { T::notch, 8000.0, 0.0, 40.0, 0 },     { T::notch, 20900.0, 0.0, 0.025, 0 },
            { T::notch, 25000.0, 0.0, 4.0, 0 },     { T::notch, 300.0, 6.0, 0.3, 0 },

            { T::bandPass, 50.0, 0.0, 1.0, 0 },     { T::bandPass, 999.99988, 0.0, 0.99999976, 0 },
            { T::bandPass, 8000.0, 0.0, 40.0, 0 },  { T::bandPass, 19000.0, 0.0, 0.025, 0 },
            { T::bandPass, 25000.0, 0.0, 4.0, 0 },  { T::bandPass, 300.0, -6.0, 0.3, 0 }
        };

        int bad = 0, total = 0;

        for (const FixedDesign& f : settings)
        {
            const eq::BandDesign reference = designAt (44100.0, mk (f.type, f.fc, f.gain, f.q, 0));

            for (int si = 1; si < 8; ++si)
            {
                ++total;

                if (! sameDesign (reference, designAt (44100.0, mk (f.type, f.fc, f.gain, f.q, si)), true))
                    ++bad;
            }
        }

        row (bad == 0 && total == 140, "SL1 D6: Bell / Notch / Band Pass bit-identical across the 8 slope indices",
             strf ("%d/%d comparisons, 20 settings", total - bad, total));
    }

    void rowQ1()
    {
        int bad = 0, total = 0;

        for (const eq::Type type : { eq::Type::lowShelf, eq::Type::highShelf, eq::Type::lowCut, eq::Type::highCut, eq::Type::tilt })
            for (const double fc : { 100.0, 999.99988, 15000.0, 25000.0 })
                for (const double gain : { -12.0, 6.0 })
                {
                    const eq::BandDesign reference = designAt (44100.0, mk (type, fc, gain, 1.0, 0));

                    for (const double q : { 0.025, 40.0 })
                    {
                        ++total;

                        if (! sameDesign (reference, designAt (44100.0, mk (type, fc, gain, q, 0)), true))
                            ++bad;
                    }
                }

        row (bad == 0 && total == 80, "Q1 D9: at 6 dB/oct the five slope types are bit-identical across Q 0.025 / 1 / 40",
             strf ("%d/%d comparisons", total - bad, total));
    }

    void rowTL1()
    {
        const double fs = 44100.0;
        double worstDc = 0.0, worstLimit = 0.0, worstPivotRatio = 0.0, worstPivot = 0.0;
        int cells = 0, pivots = 0;

        for (int si = 0; si < 8; ++si)
            for (const double gain : { 6.0, -6.0, 18.0, -18.0 })
                for (const double fc : { 100.0, 1000.0, 10000.0 })
                {
                    const eq::BandDesign d = designAt (fs, mk (eq::Type::tilt, fc, gain, 1.0, si));
                    double dc = 1.0, limit = 1.0;

                    for (int i = 0; i < d.numSections; ++i)
                    {
                        const eq::SectionParams& p = d.params[ix (i)];
                        const eq::AnalogSection& a = d.trueProto[ix (i)];
                        const double ratio = a.wp / a.wz;

                        dc *= (p.hasSvf ? p.mL : 1.0) * (p.hasOnePole ? p.cL : 1.0);
                        limit *= d.cls[ix (i)] == eq::SectionClass::gain2 ? a.c * ratio * ratio : a.c * ratio;
                    }

                    const double dcDb = 20.0 * std::log10 (dc);
                    const double limitDb = (d.inverted ? -1.0 : 1.0) * 20.0 * std::log10 (limit);
                    const double eDc = std::fabs (dcDb + gain / 2.0);
                    const double eLimit = std::fabs (limitDb - gain / 2.0);

                    worstDc = std::isfinite (eDc) ? std::max (worstDc, eDc) : kInf;
                    worstLimit = std::isfinite (eLimit) ? std::max (worstLimit, eLimit) : kInf;
                    ++cells;
                }

        for (const double gain : { 6.0, -6.0, 18.0, -18.0 })
            for (const double fc : geomspace (20.0, 10000.0, 10))
            {
                const eq::BandDesign d = designAt (fs, mk (eq::Type::tilt, fc, gain, 1.0, 1));
                const double pivotDb = std::fabs (eq::bandMagDb (d, 2.0 * kPi * fc / fs));
                const double tol = g1Tolerance (G1Kind::shelf12, fs, std::fabs (gain));

                worstPivot = std::isfinite (pivotDb) ? std::max (worstPivot, pivotDb) : kInf;
                worstPivotRatio = std::isfinite (pivotDb) ? std::max (worstPivotRatio, pivotDb / tol) : kInf;
                ++pivots;
            }

        row (worstDc <= 1.0e-9 && cells == 96, "TL1 D8: tilt design DC gain is -g/2 (8 slopes, g +-6 / +-18)",
             strf ("worst %.3g dB", worstDc));
        row (worstLimit <= 1.0e-9, "TL1 D8: true prototype high-frequency limit is +g/2",
             strf ("worst %.3g dB", worstLimit));
        row (worstPivotRatio <= 1.0 && pivots == 40, "TL1 D8: 0 dB at the pivot (12 dB/oct, Q 1, fc <= 10 kHz, 44.1 kHz)",
             strf ("worst %.4f dB, %.2f of the G1-sh12 tolerance", worstPivot, worstPivotRatio));
    }

    void rowZ1()
    {
        const std::vector<double> fcs = geomspace (10.0, 30000.0, 30);
        double worst = 0.0;
        long long sections = 0, exact = 0;
        int designs = 0;

        for (const double fs : { 44100.0, 192000.0 })
        {
            const Grid grid = makeGrid (fs, 1000, 20.0, 20000.0, 0.499);

            for (const eq::Type type : { eq::Type::bell, eq::Type::lowShelf, eq::Type::highShelf, eq::Type::tilt })
                for (int si = 0; si < 8; ++si)
                    for (const double q : { 0.025, 0.99999976, 40.0 })
                        for (const double fc : fcs)
                        {
                            const eq::BandDesign d = designAt (fs, mk (type, fc, 0.0, q, si));
                            ++designs;

                            for (const double w : grid.w)
                            {
                                const double e = std::fabs (eq::bandMagDb (d, w));

                                if (! (e <= worst))
                                    worst = std::isfinite (e) ? e : kInf;
                            }

                            for (int i = 0; i < d.numSections; ++i)
                            {
                                const eq::SectionParams& p = d.params[ix (i)];
                                const bool svfIdentity = ! p.hasSvf || (p.mL == 1.0 && p.mH == 1.0 && sameBits (p.mB, p.k));
                                const bool poleIdentity = ! p.hasOnePole || (p.cL == 1.0 && p.cH == 1.0);
                                ++sections;

                                if (svfIdentity && poleIdentity)
                                    ++exact;
                            }
                        }
        }

        row (worst <= 1.0e-12 && designs == 5760, "Z1 gain exactly 0 dB: response within 1e-12 dB of 0 on the grid",
             strf ("worst %.3g dB, %d designs; parameters bit-exact identity in %lld of %lld sections (%s)",
                   worst, designs, exact, sections, exact == sections ? "ALL" : "NOT all"));
    }

    void rowP1()
    {
        const double fs = 44100.0;

        // Bell re-centre boundary.
        const double fL = eq::bellRecentre * fs / 2.0;
        double worstRel = 0.0;

        for (const double q : { 0.3, 1.0, 4.0, 40.0 })
            for (const double gain : { -18.0, -6.0, 6.0, 18.0, 30.0 })
            {
                const eq::BandDesign below = designAt (fs, mk (eq::Type::bell, fL * (1.0 - 1.0e-9), gain, q, 1));
                const eq::BandDesign above = designAt (fs, mk (eq::Type::bell, fL * (1.0 + 1.0e-9), gain, q, 1));
                const eq::SectionParams& a = below.params[0];
                const eq::SectionParams& b = above.params[0];
                const double av[8] = { a.g, a.k, a.mL, a.mB, a.mH, a.G, a.cL, a.cH };
                const double bv[8] = { b.g, b.k, b.mL, b.mB, b.mH, b.G, b.cL, b.cH };

                for (int c = 0; c < 8; ++c)
                {
                    const double rel = std::fabs (av[c] - bv[c]) / std::max (std::fabs (av[c]), kTiny);

                    if (! (rel <= worstRel))
                        worstRel = std::isfinite (rel) ? rel : kInf;
                }
            }

        row (worstRel <= 1.0e-6, "P1 D10: bell parameters continuous across 0.95 pi (fc x (1 +- 1e-9))",
             strf ("worst relative change %.3g (limit 1e-6)", worstRel));

        // Clamps. The reference is designed a hair above the clamp frequency, where the
        // clamp is certainly active; whether the design AT the clamp frequency is the same
        // bits depends on one rounding of 2 pi f / fs and is printed, not gated.
        struct Clamp { eq::Type type; double fraction; double fcs[3]; const char* label; };
        static const Clamp clamps[2] =
        {
            { eq::Type::notch,    eq::notchClamp,    { 21000.0, 25000.0, 30000.0 }, "notch" },
            { eq::Type::bandPass, eq::bandPassClamp, { 20000.0, 25000.0, 30000.0 }, "band pass" }
        };

        int bad = 0, total = 0;
        std::string atClamp;

        for (const Clamp& clamp : clamps)
        {
            const double clampHz = clamp.fraction * fs / 2.0;
            int exactAtClamp = 0;

            for (const double q : { 0.025, 0.99999976, 40.0 })
            {
                const eq::BandDesign reference = designAt (fs, mk (clamp.type, clampHz * (1.0 + 1.0e-9), 0.0, q, 1));

                for (const double fc : clamp.fcs)
                {
                    ++total;

                    if (! sameDesign (reference, designAt (fs, mk (clamp.type, fc, 0.0, q, 1)), false))
                        ++bad;
                }

                if (sameDesign (reference, designAt (fs, mk (clamp.type, clampHz, 0.0, q, 1)), false))
                    ++exactAtClamp;
            }

            atClamp += strf ("%s%s at exactly %.1f Hz: %d/3 same bits", atClamp.empty() ? "" : "; ", clamp.label, clampHz, exactAtClamp);
        }

        row (bad == 0 && total == 18, "P1 D10: notch 21/25/30 k and band pass 20/25/30 k == the design at the clamp (memcmp)",
             strf ("%d/%d; %s", total - bad, total, atClamp.c_str()));

        // Two-sided (VERIFICATION-2.1 issue 5): where the clamps ARE, measured from
        // behaviour with harness-local literals. eq::notchClamp and eq::bandPassClamp
        // are deliberately not read below. The "at the clamp" reference is designed a
        // hair above the literal, for the rounding reason given above.
        {
            const double notchLiteral = 0.95;       // x pi
            const double bandPassLiteral = 0.9;     // x pi
            const double nyquist = fs / 2.0;
            int checks = 0, failed = 0;
            std::string firstFailure;

            const auto check = [&] (bool ok, const char* what, double q)
            {
                ++checks;

                if (! ok)
                {
                    ++failed;

                    if (firstFailure.empty())
                        firstFailure = strf ("%s (Q %g)", what, q);
                }
            };

            for (const double q : { 0.3, 1.0, 4.0, 40.0 })
            {
                const double fBelow = (notchLiteral - 0.001) * nyquist;
                const double fAbove = (notchLiteral + 0.001) * nyquist;
                const eq::BandDesign below = designAt (fs, mk (eq::Type::notch, fBelow, 0.0, q, 1));
                const eq::BandDesign above = designAt (fs, mk (eq::Type::notch, fAbove, 0.0, q, 1));
                const eq::BandDesign hair  = designAt (fs, mk (eq::Type::notch, notchLiteral * nyquist * (1.0 + 1.0e-9), 0.0, q, 1));

                check (! sameDesign (below, above, false), "notch at 0.949 pi is the same design as at 0.951 pi", q);
                check (eq::bandMagDb (below, 2.0 * kPi * fBelow / fs) < -100.0, "notch at 0.949 pi has no null at 0.949 pi", q);
                check (sameDesign (above, hair, false), "notch at 0.951 pi differs from the design a hair above 0.95 pi", q);
                check (eq::bandMagDb (above, notchLiteral * kPi) < -100.0, "notch at 0.951 pi has no null at 0.95 pi", q);
                check (eq::bandMagDb (above, 2.0 * kPi * fAbove / fs) > -100.0, "notch at 0.951 pi has a null at 0.951 pi", q);
            }

            for (const double q : { 0.3, 1.0, 4.0, 40.0 })
            {
                const eq::BandDesign below = designAt (fs, mk (eq::Type::bandPass, (bandPassLiteral - 0.001) * nyquist, 0.0, q, 1));
                const eq::BandDesign above = designAt (fs, mk (eq::Type::bandPass, (bandPassLiteral + 0.001) * nyquist, 0.0, q, 1));
                const eq::BandDesign hair  = designAt (fs, mk (eq::Type::bandPass, bandPassLiteral * nyquist * (1.0 + 1.0e-9), 0.0, q, 1));

                check (! sameDesign (below, above, false), "band pass at 0.899 pi is the same design as at 0.901 pi", q);
                check (sameDesign (above, hair, false), "band pass at 0.901 pi differs from the design a hair above 0.9 pi", q);
            }

            // Where the peak sits: strictly above both neighbours 0.001 pi away.
            for (const double q : { 4.0, 40.0 })
            {
                const eq::BandDesign below = designAt (fs, mk (eq::Type::bandPass, (bandPassLiteral - 0.001) * nyquist, 0.0, q, 1));
                const eq::BandDesign above = designAt (fs, mk (eq::Type::bandPass, (bandPassLiteral + 0.001) * nyquist, 0.0, q, 1));
                const double wLow2 = (bandPassLiteral - 0.002) * kPi;
                const double wLow1 = (bandPassLiteral - 0.001) * kPi;
                const double wAt   = bandPassLiteral * kPi;
                const double wHigh = (bandPassLiteral + 0.001) * kPi;

                check (eq::bandMagDb (below, wLow1) > eq::bandMagDb (below, wLow2)
                         && eq::bandMagDb (below, wLow1) > eq::bandMagDb (below, wAt),
                       "band pass at 0.899 pi does not peak at 0.899 pi", q);
                check (eq::bandMagDb (above, wAt) > eq::bandMagDb (above, wLow1)
                         && eq::bandMagDb (above, wAt) > eq::bandMagDb (above, wHigh),
                       "band pass at 0.901 pi does not peak at 0.9 pi", q);
            }

            row (failed == 0 && checks == 32,
                 "P1 D10 two-sided: the notch clamp is AT 0.95 pi and the band-pass clamp AT 0.9 pi (harness literals)",
                 strf ("%d/%d checks: design changes below the clamp, is fixed above it, null / peak at the clamp%s%s",
                       checks - failed, checks, firstFailure.empty() ? "" : "; first failure: ", firstFailure.c_str()));
        }
    }

    //==========================================================================
    // P1s: the shelf re-centre of D10, measured from behaviour with
    // harness-local literals. eq::shelfRecentre, eq::shelfRecentreQLo,
    // eq::shelfRecentreQHi, eq::recentreQMin and eq::recentreQMax are
    // deliberately not read here.
    //==========================================================================
    /** Second-order section number i of a shelf / tilt band as the Phase 2.1
        prototype defines it, written out independently of Prototype.h.
        False when the band has no such section. */
    bool p1sTrueSection (const eq::BandSettings& s, double fs, int i, eq::AnalogSection& out)
    {
        static const int orders[8] = { 1, 2, 3, 4, 6, 8, 12, 16 };
        const int order = orders[s.slopeIndex];

        if (i < 0 || i >= order / 2)
            return false;

        const double orderD = static_cast<double> (order);
        const double wc = 2.0 * kPi * s.freqHz / fs;
        const double Gl = std::pow (10.0, std::fabs (s.gainDbEff) / 20.0);
        const double r = std::pow (Gl, 1.0 / (2.0 * orderD));
        const double angle = (static_cast<double> (2 * i + 1) * kPi) / (2.0 * orderD);
        const double Qi = 1.0 / (2.0 * std::sin (angle));
        const double Q = (i == 0) ? Qi * s.q : Qi;

        if (s.type == eq::Type::lowShelf)
            out = eq::AnalogSection { wc * r, Q, wc / r, Q, std::pow (Gl, 2.0 / orderD) };
        else
            out = eq::AnalogSection { wc / r, Q, wc * r, Q, 1.0 };

        return true;
    }

    /** Largest relative difference over the eight parameters of two sections. */
    double paramsRelativeChange (const eq::SectionParams& a, const eq::SectionParams& b)
    {
        const double av[8] = { a.g, a.k, a.mL, a.mB, a.mH, a.G, a.cL, a.cH };
        const double bv[8] = { b.g, b.k, b.mL, b.mB, b.mH, b.G, b.cL, b.cH };
        double worst = 0.0;

        for (int c = 0; c < 8; ++c)
        {
            const double rel = std::fabs (av[c] - bv[c]) / std::max (std::fabs (av[c]), kTiny);

            if (! (rel <= worst))
                worst = std::isfinite (rel) ? rel : kInf;
        }

        return worst;
    }

    void rowP1s()
    {
        const double boundary = 0.95;       // x pi
        const double qLo = 1.5, qHi = 4.0;  // pair Q: no move at or below qLo, the whole move at or above qHi
        const double clampLo = 0.025, clampHi = 160.0;
        const double wL = boundary * kPi;

        const auto relative = [] (double a, double b)
        {
            const double rel = std::fabs (a - b) / std::max (std::fabs (a), kTiny);
            return std::isfinite (rel) ? rel : kInf;
        };

        // The upper pair of the one second-order section of a 12 dB/oct shelf:
        // the pole pair of a High Shelf, the zero pair of a Low Shelf.
        struct Pair { double trueW, trueQ, designW, designQ; };

        const auto upperPair = [] (const eq::BandSettings& s, double rate)
        {
            eq::AnalogBand proto;
            eq::buildPrototype (s, rate, proto);
            const eq::AnalogSection& t = proto.trueSide[0];
            const eq::AnalogSection& d = proto.designSide[0];
            return s.type == eq::Type::lowShelf ? Pair { t.wz, t.Qz, d.wz, d.Qz } : Pair { t.wp, t.Qp, d.wp, d.Qp };
        };

        // (i), (ii): continuity at the three boundaries, on designSeed and on the design-side pair.
        {
            const double fs = 44100.0;
            double worstParam = 0.0, worstPair = 0.0;
            int crossings = 0, crossingsBad = 0, edges = 0, edgesBad = 0;

            const auto compare = [&] (const eq::BandSettings& sBelow, const eq::BandSettings& sAbove,
                                      const Pair& pairBelow, const Pair& pairAbove)
            {
                worstParam = std::max (worstParam, paramsRelativeChange (designAt (fs, sBelow).params[0],
                                                                         designAt (fs, sAbove).params[0]));
                worstPair = std::max (worstPair, std::max (relative (pairBelow.designW, pairAbove.designW),
                                                           relative (pairBelow.designQ, pairAbove.designQ)));
            };

            // (i) the pair's frequency crossing 0.95 pi.
            for (const eq::Type type : { eq::Type::highShelf, eq::Type::lowShelf })
                for (const double q : { 4.0, 40.0 })
                    for (const double gain : { 6.0, 18.0 })
                    {
                        const double r = std::pow (std::pow (10.0, gain / 20.0), 0.25);
                        const double fAt = boundary * (fs / 2.0) / r;       // the upper pair, wc r, sits on 0.95 pi
                        const eq::BandSettings below = mk (type, fAt * (1.0 - 1.0e-9), gain, q, 1);
                        const eq::BandSettings above = mk (type, fAt * (1.0 + 1.0e-9), gain, q, 1);
                        const Pair pb = upperPair (below, fs);
                        const Pair pa = upperPair (above, fs);
                        compare (below, above, pb, pa);
                        ++crossings;

                        // The crossing is real: untouched under the boundary, over it on the other side.
                        if (! (pb.trueW < wL && pa.trueW > wL && sameBits (pb.designW, pb.trueW) && sameBits (pb.designQ, pb.trueQ)))
                            ++crossingsBad;
                    }

            // (ii) the pair's Q crossing 1.5 and 4 with the pair above 0.95 pi.
            const double q12 = 1.0 / (2.0 * std::sin (kPi / 4.0));     // section Q = q12 x the band's Q at 12 dB/oct

            for (const eq::Type type : { eq::Type::highShelf, eq::Type::lowShelf })
                for (const double fc : { 21500.0, 25000.0 })
                    for (const double gain : { 6.0, 18.0 })
                    {
                        const auto at = [&] (double sectionQ)
                        {
                            return mk (type, fc, gain, sectionQ / q12, 1);
                        };

                        for (const double edge : { qLo, qHi })
                        {
                            const eq::BandSettings below = at (edge * (1.0 - 1.0e-9));
                            const eq::BandSettings above = at (edge * (1.0 + 1.0e-9));
                            const Pair pb = upperPair (below, fs);
                            const Pair pa = upperPair (above, fs);
                            compare (below, above, pb, pa);
                            ++edges;

                            // Under 1.5 the pair is untouched; over 4 it sits on 0.95 pi.
                            const bool pairIsAbove = pb.trueW > wL && pa.trueW > wL;
                            const bool edgeOk = edge < 2.0 ? (sameBits (pb.designW, pb.trueW) && sameBits (pb.designQ, pb.trueQ))
                                                           : relative (wL, pa.designW) <= 1.0e-9;

                            if (! (pairIsAbove && edgeOk))
                                ++edgesBad;
                        }

                        // Between the two the move is partial and grows with Q.
                        const Pair early = upperPair (at (1.8), fs);
                        const Pair late = upperPair (at (3.3), fs);
                        ++edges;

                        if (! (early.designW < early.trueW * (1.0 - 1.0e-6) && late.designW > wL * (1.0 + 1.0e-6)
                                && late.designW < early.designW))
                            ++edgesBad;
                    }

            row (worstParam <= 1.0e-6 && worstPair <= 1.0e-6 && crossings == 8 && crossingsBad == 0 && edges == 24 && edgesBad == 0,
                 "P1s D10 shelf re-centre continuous across 0.95 pi and across pair Q 1.5 and 4 (x (1 +- 1e-9), harness literals)",
                 strf ("worst relative change: seed parameters %.3g, design-side pair %.3g (limit 1e-6); %d/%d frequency crossings, "
                       "%d/%d Q-boundary checks",
                       worstParam, worstPair, crossings - crossingsBad, crossings, edges - edgesBad, edges));
        }

        // (iii), (iv): the region and the moved pairs, on buildPrototype.
        {
            constexpr int numSettings = 10000;
            constexpr double slack = 1.0e-12;       // exp (log (x)) can land an ulp either side of x
            static const eq::Type shelfTypes[3] = { eq::Type::lowShelf, eq::Type::highShelf, eq::Type::tilt };
            Rng rng (0x50315331ull);
            long long pairs = 0, moved = 0, outsideBad = 0, trueBad = 0, otherBad = 0, movedBad = 0, magnitudeChecked = 0;
            double worstMagnitude = 0.0;
            std::string first;

            const auto problem = [&first] (const eq::BandSettings& s, double rate, int section, const char* what)
            {
                if (first.empty())
                    first = describeSettings (s) + strf (" fs %.0f section %d: %s", rate, section, what);
            };

            for (int n = 0; n < numSettings; ++n)
            {
                const eq::Type type = shelfTypes[rng.below (3)];
                const int slope = rng.below (8);
                const double fc = 10.0 * std::pow (3000.0, rng.uniform());
                const double q = 0.025 * std::pow (1600.0, rng.uniform());
                const double gain = -60.0 + 120.0 * rng.uniform();
                const eq::BandSettings s = mk (type, fc, gain, q, slope);

                for (const double fs : kRates8)
                {
                    eq::AnalogBand proto;
                    eq::buildPrototype (s, fs, proto);

                    for (int i = 0; i < proto.numSections; ++i)
                    {
                        const eq::AnalogSection& d = proto.designSide[ix (i)];
                        const eq::AnalogSection& t = proto.trueSide[ix (i)];

                        if (proto.cls[ix (i)] != eq::SectionClass::gain2)
                        {
                            if (std::memcmp (&d, &t, sizeof (eq::AnalogSection)) != 0)
                            {
                                ++otherBad;
                                problem (s, fs, i, "the first-order section's design side differs from its true side");
                            }

                            continue;
                        }

                        eq::AnalogSection expected {};

                        if (! p1sTrueSection (s, fs, i, expected) || std::memcmp (&t, &expected, sizeof (eq::AnalogSection)) != 0)
                        {
                            ++trueBad;
                            problem (s, fs, i, "the true side is not the Phase 2.1 section");
                        }

                        if (! sameBits (d.c, t.c))
                        {
                            ++otherBad;
                            problem (s, fs, i, "the design side's DC gain differs");
                        }

                        const double tw[2] = { t.wz, t.wp }, tq[2] = { t.Qz, t.Qp };
                        const double dw[2] = { d.wz, d.wp }, dq[2] = { d.Qz, d.Qp };

                        for (int p = 0; p < 2; ++p)
                        {
                            ++pairs;
                            const bool same = sameBits (dw[p], tw[p]) && sameBits (dq[p], tq[p]);

                            if (tw[p] <= wL || tq[p] <= qLo)
                            {
                                if (! same)
                                {
                                    ++outsideBad;
                                    problem (s, fs, i, "a pair at or below 0.95 pi or Q 1.5 was changed");
                                }

                                continue;
                            }

                            if (same)
                                continue;

                            ++moved;
                            bool ok = dw[p] >= wL * (1.0 - slack) && dw[p] <= tw[p] * (1.0 + slack)
                                   && dq[p] >= clampLo && dq[p] <= clampHi;

                            if (dq[p] > clampLo && dq[p] < clampHi)
                            {
                                // Both with DC value 1: the true pair at wT, the moved pair at its own frequency.
                                const double x = dw[p] / tw[p];
                                const double x2 = x * x;
                                const double oneMinusX2 = 1.0 - x2;
                                const double truePower = oneMinusX2 * oneMinusX2 + x2 / (tq[p] * tq[p]);
                                const double ownPower = 1.0 / (dq[p] * dq[p]);
                                const double rel = relative (truePower, ownPower);
                                ++magnitudeChecked;
                                worstMagnitude = std::max (worstMagnitude, rel);

                                if (! (rel <= 1.0e-12))
                                    ok = false;
                            }

                            if (! ok)
                            {
                                ++movedBad;
                                problem (s, fs, i, "a moved pair is outside [0.95 pi, w0], outside the Q clamp, or off in magnitude at wT");
                            }
                        }
                    }
                }
            }

            row (outsideBad == 0 && trueBad == 0 && otherBad == 0 && movedBad == 0 && moved >= 1000 && magnitudeChecked > 0,
                 "P1s D10 shelf re-centre region: 10 000 random shelf / tilt settings x 8 rates (buildPrototype, harness literals)",
                 strf ("%lld pairs: %lld changed outside the region, %lld true sections off the Phase 2.1 formula, %lld other differences; "
                       "%lld moved pairs%s, %lld of them bad (0.95 pi <= wT <= w0, Q clamp, magnitude at wT: worst %.3g relative over %lld, "
                       "limit 1e-12)",
                       pairs, outsideBad, trueBad, otherBad, moved, moved >= 1000 ? "" : " (VOID: under 1000)", movedBad,
                       worstMagnitude, magnitudeChecked)
                     + (first.empty() ? std::string() : "  first: " + first));
        }
    }

    //==========================================================================
    // NB1-NB4 and the notch / band-pass INFO cells
    // (research-2.1/t2_g1_notch_bp.py: 4000 log points 20 Hz - min (20 kHz, 0.4999 fs),
    // no tied points, 40 centres per zone from lo x 1.0001, 25 Q, error counted where
    // the TRUE prototype is above -40 dB)
    //==========================================================================
    double notchBandPassZone (double fs, eq::Type type, double fLo, double fHi, const Grid& grid)
    {
        return zoneWorst (fs, { type }, 1, geomspace (fLo * 1.0001, fHi, 40), geomspace (0.025, 40.0, 25),
                          { 0.0 }, grid, false, -40.0);
    }

    void rowsNB (std::vector<std::string>& infoLines)
    {
        struct Gate { const char* name; eq::Type type; int zone; double ceiling; double model; };
        static const Gate gates[4] =
        {
            { "NB1 notch, fc <= 10 k, 44.1 kHz",                    eq::Type::notch,    0, 0.75, 0.63 },
            { "NB2 notch, 10 k - clamp (0.95 Nyquist), 44.1 kHz",   eq::Type::notch,    1, 1.45, 1.27 },
            { "NB3 band pass, fc <= 10 k, 44.1 kHz",                eq::Type::bandPass, 0, 1.65, 1.45 },
            { "NB4 band pass, 10 k - clamp (0.9 Nyquist), 44.1 kHz", eq::Type::bandPass, 1, 3.30, 2.92 }
        };

        for (const double fs : { 44100.0, 48000.0 })
        {
            const Grid grid = makeGrid (fs, 4000, 20.0, 20000.0, 0.4999);

            for (const eq::Type type : { eq::Type::notch, eq::Type::bandPass })
            {
                const double clampHz = (type == eq::Type::notch ? eq::notchClamp : eq::bandPassClamp) * fs / 2.0;
                const double edges[5] = { 20.0, 10000.0, clampHz, 25000.0, 30000.0 };
                double zone[4] = {};

                for (int z = 0; z < 4; ++z)
                    zone[z] = notchBandPassZone (fs, type, edges[z], edges[z + 1], grid);

                if (fs < 46000.0)
                    for (const Gate& gate : gates)
                        if (gate.type == type)
                            row (zone[gate.zone] <= gate.ceiling, gate.name,
                                 strf ("%.3f dB (ceiling %.2f, model %.2f)", zone[gate.zone], gate.ceiling, gate.model));

                infoLines.push_back (strf ("%s %.1f k: fc<=10k %.2f | 10k-clamp %.2f | clamp-25k vs TRUE prototype %.2f | 25-30k %.2f dB",
                                           type == eq::Type::notch ? "notch    " : "band pass", fs / 1000.0,
                                           zone[0], zone[1], zone[2], zone[3]));
            }
        }
    }

    //==========================================================================
    // A1: allocation
    //==========================================================================
   #if defined (__APPLE__)
    // volatile: the compiler knows the allocator as a builtin that cannot touch
    // globals and would otherwise fold "armed = true; ...; armed = false".
    volatile bool allocArmed = false;
    volatile int allocCount = 0;
    volatile int freeCount = 0;
    pthread_t allocThread;

    void countAllocation (uint32_t type, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uint32_t)
    {
        if (! allocArmed || pthread_equal (pthread_self(), allocThread) == 0)
            return;

        if ((type & 2u) != 0)       // MALLOC_LOG_TYPE_ALLOCATE
            allocCount = allocCount + 1;

        if ((type & 4u) != 0)       // MALLOC_LOG_TYPE_DEALLOCATE
            freeCount = freeCount + 1;
    }
   #endif

    struct AllocResult
    {
        bool available = false;
        int allocs = 0, frees = 0;
        int vacuityAllocs = 0, vacuityFrees = 0;
        long long checksum = 0;
    };

    /** 1e5 design calls inside a malloc_logger window on this thread. `plant`
        puts one deliberate allocation inside the window (the negative control).
        A separate short window with a deliberate allocation proves the hook live.
        The seed designer (A1) or the polished one (A2). */
    AllocResult runA1 (bool plant, bool polished = false)
    {
        AllocResult result;

       #if defined (__APPLE__)
        constexpr int numCalls = 100000;
        std::vector<eq::BandSettings> settings;
        settings.reserve (ix (numCalls));
        Rng rng (0xA110CA7Eull);

        for (int i = 0; i < numCalls; ++i)
            settings.push_back (randomSettings (rng));

        std::array<eq::Designer, 4> designers;
        designers[0].prepare (44100.0);
        designers[1].prepare (48000.0);
        designers[2].prepare (96000.0);
        designers[3].prepare (192000.0);

        eq::BandDesign d {};
        long long checksum = 0;

        malloc_logger_t* const previous = malloc_logger;
        allocThread = pthread_self();
        allocCount = 0;
        freeCount = 0;
        malloc_logger = countAllocation;

        // Vacuity: the hook must see a deliberate allocation and its release.
        allocArmed = true;
        {
            void* volatile planted = std::malloc (64);
            std::free (planted);
        }
        allocArmed = false;
        result.vacuityAllocs = allocCount;
        result.vacuityFrees = freeCount;
        allocCount = 0;
        freeCount = 0;

        allocArmed = true;

        for (int i = 0; i < numCalls; ++i)
        {
            designBand (designers[ix (i & 3)], polished, settings[ix (i)], d);
            checksum += d.numSections;
        }

        if (plant)
        {
            void* volatile planted = std::malloc (64);
            std::free (planted);
        }

        allocArmed = false;
        malloc_logger = previous;

        result.available = true;
        result.allocs = allocCount;
        result.frees = freeCount;
        result.checksum = checksum;
       #else
        (void) plant;
        (void) polished;
       #endif

        return result;
    }

    /** A1 (seed) and A2 (polished). */
    void rowAllocation (bool polished)
    {
        const AllocResult r = runA1 (false, polished);

        if (! r.available)
        {
            info (polished ? "A2 allocation gate SKIPPED: no malloc_logger on this platform"
                           : "A1 allocation gate SKIPPED: no malloc_logger on this platform");
            return;
        }

        const bool hookLive = r.vacuityAllocs >= 1 && r.vacuityFrees >= 1;
        row (hookLive && r.allocs == 0 && r.frees == 0,
             polished ? "A2 1e5 design() calls, polished: 0 allocations, 0 frees (malloc_logger, this thread)"
                      : "A1 1e5 designSeed() calls: 0 allocations, 0 frees (malloc_logger, this thread)",
             strf ("%d allocations, %d frees; hook %s (planted: %d / %d); %lld sections designed",
                   r.allocs, r.frees, hookLive ? "live" : "DEAD", r.vacuityAllocs, r.vacuityFrees, r.checksum));
    }

    //==========================================================================
    // B1: two threads
    //==========================================================================
    /** B1 (seed) and B2 (polished). */
    void rowTwoThreads (bool polished)
    {
        constexpr int numSettings = 10000;
        std::vector<eq::BandSettings> settings;
        Rng rng (0xB1B1B1B1ull);

        for (int i = 0; i < numSettings; ++i)
            settings.push_back (randomSettings (rng));

        std::vector<eq::BandDesign> onMain (ix (numSettings)), onSecond (ix (numSettings));

        const auto work = [&settings, polished] (std::vector<eq::BandDesign>& out)
        {
            std::array<eq::Designer, 5> designers;      // this thread's own

            for (std::size_t r = 0; r < kRates5.size(); ++r)
                designers[r].prepare (kRates5[r]);

            for (std::size_t i = 0; i < settings.size(); ++i)
                designBand (designers[i % kRates5.size()], polished, settings[i], out[i]);
        };

        work (onMain);
        std::thread second (work, std::ref (onSecond));
        second.join();

        int bad = 0;

        for (int i = 0; i < numSettings; ++i)
            if (! sameDesign (onMain[ix (i)], onSecond[ix (i)], true))
                ++bad;

        row (bad == 0,
             polished ? "B2 polished, 10 000 mixed settings: main thread == second thread with its own Designer (memcmp)"
                      : "B1 10 000 mixed settings: main thread == second thread with its own Designer (memcmp)",
             strf ("%d/%d (used parameters, weights, true prototypes)", numSettings - bad, numSettings));
    }

    //==========================================================================
    // INFO block
    //==========================================================================
    void infoRows (const std::vector<std::string>& notchBandPassLines)
    {
        const double fs = 44100.0;
        const Grid grid = makeGrid (fs, 1000, 20.0, 20000.0, 0.499);
        const std::vector<double> lowFcs = geomspace (20.0, 10000.0, 20);
        const std::vector<double> highFcs = geomspace (10000.0 * 1.0001, 20000.0, 20);
        const std::vector<double> q1 { 1.0 };
        static const double gains[3] = { 6.0, 12.0, 30.0 };

        const auto triple = [&] (std::initializer_list<eq::Type> types, int slopeIndex, const std::vector<double>& fcs,
                                 const std::vector<double>& qs, double floorDb)
        {
            std::string text;

            for (const double gainAbs : gains)
                text += strf ("%s%.2f", text.empty() ? "" : " / ",
                              zoneWorst (fs, types, slopeIndex, fcs, qs, { gainAbs, -gainAbs }, grid, true, floorDb));

            return text;
        };

        const auto shelves = { eq::Type::lowShelf, eq::Type::highShelf };

        info ("notch / band pass against the TRUE prototype (RESEARCH-2.1 Q2), error where it is above -40 dB");

        for (const std::string& text : notchBandPassLines)
            note (text);

        info ("resonant shelves 12 dB/oct at 44.1 kHz, +-6 / +-12 / +-30 dB (RESEARCH-2.1 Q8 weak rows)");
        note ("Q 2-10   fc <= 10 k: " + triple (shelves, 1, lowFcs, geomspace (2.0, 10.0, 5), -1.0e9)
                  + "   10-20 k: " + triple (shelves, 1, highFcs, geomspace (2.0, 10.0, 5), -1.0e9));
        note ("Q 20-40  fc <= 10 k: " + triple (shelves, 1, lowFcs, geomspace (20.0, 40.0, 3), -1.0e9)
                  + "   10-20 k: " + triple (shelves, 1, highFcs, geomspace (20.0, 40.0, 3), -1.0e9));

        info ("shelves >= 24 dB/oct, Q 1, fc 10-20 k at 44.1 kHz, +-6 / +-12 / +-30 dB (model 0.63/1.24/2.31, 0.84/1.85/4.33, 0.87/1.77/5.04)");
        note ("24 dB/oct: " + triple (shelves, 3, highFcs, q1, -1.0e9)
                  + "   48 dB/oct: " + triple (shelves, 5, highFcs, q1, -1.0e9)
                  + "   96 dB/oct: " + triple (shelves, 7, highFcs, q1, -1.0e9));

        const auto cut = [&] (eq::Type type, int slopeIndex, const std::vector<double>& fcs, double floorDb)
        {
            return strf ("%.2f", zoneWorst (fs, { type }, slopeIndex, fcs, q1, { 0.0 }, grid, true, floorDb));
        };

        info ("cut skirts at 44.1 kHz, Q 1 (ARCHITECTURE weak spots: 1.1; 2.1 / 1.2; 3.6 / 5.7; 0.09 / 0.00 / 0.00)");
        note ("high cut 12 dB/oct, fc <= 10 k, down to -80 dB: " + cut (eq::Type::highCut, 1, lowFcs, -80.0));
        note ("high cut 48 / 96 dB/oct, top 30 dB, fc <= 10 k: " + cut (eq::Type::highCut, 5, lowFcs, -30.0)
                  + " / " + cut (eq::Type::highCut, 7, lowFcs, -30.0)
                  + "   fc 10-20 k: " + cut (eq::Type::highCut, 5, highFcs, -30.0)
                  + " / " + cut (eq::Type::highCut, 7, highFcs, -30.0));
        note ("low cut 12 / 48 / 96 dB/oct, fc <= 10 k, down to -80 dB: " + cut (eq::Type::lowCut, 1, lowFcs, -80.0)
                  + " / " + cut (eq::Type::lowCut, 5, lowFcs, -80.0) + " / " + cut (eq::Type::lowCut, 7, lowFcs, -80.0));

        // Seed design time per section.
        constexpr int numBells = 400000;
        std::vector<eq::AnalogSection> sections;
        sections.reserve (ix (numBells));
        Rng rng (0x5EEDull);

        for (int i = 0; i < numBells; ++i)
        {
            const double fc = 20.0 * std::pow (1000.0, rng.uniform());
            const double q = 0.025 * std::pow (1600.0, rng.uniform());
            const double A = std::pow (10.0, (1.0 + 29.0 * rng.uniform()) / 40.0);
            const double wc = 2.0 * kPi * fc / fs;
            sections.push_back (eq::AnalogSection { wc, q / A, wc, q * A, 1.0 });
        }

        double sink = 0.0;
        const auto start = std::chrono::steady_clock::now();

        for (const eq::AnalogSection& a : sections)
            sink += eq::Designer::designSeedSection (a).mB;

        const auto stop = std::chrono::steady_clock::now();
        const double ns = std::chrono::duration<double, std::nano> (stop - start).count() / static_cast<double> (numBells);

        info ("seed design time per section (400 000 random bells, 44.1 kHz)",
              strf ("%.1f ns  (research port 30-36 ns)  [checksum %.6g]", ns, sink));
    }

    //==========================================================================
    // CS1 / CS2: cut-skirt regression ceilings (VERIFICATION-2.1 issue 4)
    //==========================================================================
    void rowsCS()
    {
        const double fs = 44100.0;
        const Grid grid = makeGrid (fs, 1000, 20.0, 20000.0, 0.499);
        const std::vector<double> q1 { 1.0 };

        struct Cell { int slopeIndex; int dbPerOct; double ceiling; };
        struct Gate { const char* name; eq::Type type; double fLo, fHi; Cell cells[3]; const char* reference; };

        static const Gate gates[2] =
        {
            { "CS1 Low Cut 12 / 24 / 96 dB/oct, fc 2-20 kHz, Q 1, 44.1 kHz vs prototype above -60 dB",
              eq::Type::lowCut, 2000.0, 20000.0,
              { { 1, 12, kCs1LowCut12Ceiling }, { 3, 24, kCs1LowCut24Ceiling }, { 7, 96, kCs1LowCut96Ceiling } },
              "verify measured 1.35 / 0.82 / 0.01 with another scorer" },
            { "CS2 High Cut 12 / 48 / 96 dB/oct, fc <= 10 kHz, Q 1, 44.1 kHz vs prototype above -60 dB",
              eq::Type::highCut, 20.0, 10000.0,
              { { 1, 12, kCs2HighCut12Ceiling }, { 5, 48, kCs2HighCut48Ceiling }, { 7, 96, kCs2HighCut96Ceiling } },
              "ceilings = first measured run + 15 %" }
        };

        for (const Gate& gate : gates)
        {
            const std::vector<double> fcs = geomspace (gate.fLo, gate.fHi, 20);
            std::string text;
            bool ok = true;

            for (const Cell& cell : gate.cells)
            {
                const double worst = zoneWorst (fs, { gate.type }, cell.slopeIndex, fcs, q1, { 0.0 }, grid, true, -60.0);
                const bool cellOk = worst <= cell.ceiling;
                ok = ok && cellOk;
                text += strf ("%s%d dB/oct %.4f dB (ceiling %.2f)%s", text.empty() ? "" : " | ", cell.dbPerOct, worst,
                              cell.ceiling, cellOk ? "" : " OVER");
            }

            row (ok, gate.name, text + "; " + gate.reference);
        }
    }

    //==========================================================================
    // Phase 2.1b
    //==========================================================================
    void printPolishConstants()
    {
        namespace pc = eq::polish;
        std::string scales;

        for (int i = 0; i < pc::numTiedScales; ++i)
            scales += strf ("%s%g", i == 0 ? "" : " / ", pc::tiedScales[i]);

        note ("polish constants, read from Source/dsp/PolishConstants.h (PLAN-2.1b P10):");
        note (strf ("  fixedPoints %d, numTiedScales %d, tiedScales %s, tiedPoints %d, numPoints %d",
                    pc::fixedPoints, pc::numTiedScales, scales.c_str(), pc::tiedPoints, pc::numPoints));
        note (strf ("  steps %d, extraSteps %d, numFree %d, lambda %g, mu %g",
                    pc::steps, pc::extraSteps, pc::numFree, pc::lambda, pc::mu));
        note (strf ("  seedKeepRms %g dB, seedFullRms %g dB, extensionRms %g dB, acceptRamp %g",
                    pc::seedKeepRms, pc::seedFullRms, pc::extensionRms, pc::acceptRamp));
        note (strf ("  match band %g Hz ... min (%g Hz, %g fs)", pc::bandLowHz, pc::bandHighHz, pc::bandTopFraction));
        note (strf ("  stepLimit %g, minSampleRateHz %g Hz", pc::stepLimit, pc::minSampleRateHz));
        note (strf ("shelf re-centre constants, read from Source/dsp/Prototype.h: shelfRecentre %g pi, "
                    "shelfRecentreQLo %g, shelfRecentreQHi %g",
                    eq::shelfRecentre, eq::shelfRecentreQLo, eq::shelfRecentreQHi));
    }

    //==========================================================================
    // D2: the polished golden
    //==========================================================================
    bool parseInt (const std::string& text, int& value)
    {
        if (text.empty())
            return false;

        char* end = nullptr;
        const long parsed = std::strtol (text.c_str(), &end, 10);

        if (end != text.c_str() + text.size() || parsed < -1000000L || parsed > 1000000L)
            return false;

        value = static_cast<int> (parsed);
        return true;
    }

    void rowD2 (const std::string& goldenDir)
    {
        const char* const name = "D2 C++ design == golden designs-polished.tsv (1e-6 rel; mL cL 1e-9; weight 1e-6; response 1e-6 dB)";
        const std::string path = goldenDir + "/designs-polished.tsv";
        std::ifstream in (path, std::ios::binary);

        if (! in.is_open())
        {
            row (false, name, "cannot read " + path);
            return;
        }

        long long expectedLines = 0;

        for (const GoldenSetting& gs : polishedGoldenSettings())
            expectedLines += expectedSections (static_cast<eq::Type> (gs.type), gs.slope);

        static const char* const columnNames[8] = { "g", "k", "mL", "mB", "mH", "G", "cL", "cH" };

        eq::Designer designer;
        Grid grid;
        Tied tied;
        eq::BandDesign current {}, golden {};
        std::string headKey, key, line, firstProblem, worstWhere, worstResponseWhere;
        std::vector<std::string> cells;
        bool bandOpen = false;
        int nextSec = 0;
        double preparedFs = 0.0;
        long long lineNumber = 0, dataLines = 0, parseBad = 0, shapeBad = 0, cellsCompared = 0, cellsBad = 0;
        long long weightBad = 0, bands = 0, bandsBad = 0;
        double worstRel = 0.0, worstWeight = 0.0, worstResponse = 0.0;

        const auto problem = [&firstProblem] (const std::string& text)
        {
            if (firstProblem.empty())
                firstProblem = text;
        };

        while (std::getline (in, line))
        {
            ++lineNumber;

            if (line.empty() || line[0] == '#')
                continue;

            ++dataLines;
            splitTabs (line, cells);

            double fsCell = 0.0, fcCell = 0.0, qCell = 0.0, gainCell = 0.0, weightCell = 0.0;
            int typeIndex = -1, slope = -1, sec = -1;

            const bool headOk = cells.size() == 19
                             && parseDouble (cells[0], fsCell) && std::isfinite (fsCell) && fsCell > 0.0
                             && parseInt (cells[2], slope) && slope >= 0 && slope < 8
                             && parseDouble (cells[3], fcCell) && std::isfinite (fcCell) && fcCell > 0.0
                             && parseDouble (cells[4], qCell) && std::isfinite (qCell) && qCell > 0.0
                             && parseDouble (cells[5], gainCell) && std::isfinite (gainCell)
                             && parseInt (cells[6], sec) && sec >= 0 && sec < eq::maxSections
                             && parseDouble (cells[18], weightCell) && std::isfinite (weightCell);

            if (headOk)
                for (int t = 0; t < 8; ++t)
                    if (cells[1] == kTypeNames[t])
                        typeIndex = t;

            if (! headOk || typeIndex < 0 || ! eq::isGainType (static_cast<eq::Type> (typeIndex)))
            {
                ++parseBad;
                bandOpen = false;
                problem (strf ("line %lld does not parse", lineNumber));
                continue;
            }

            const auto where = [&]
            {
                return strf ("line %lld (%s fs %g slope-index %d fc %g q %g gain %g section %d)", lineNumber,
                             kTypeNames[typeIndex], fsCell, slope, fcCell, qCell, gainCell, sec);
            };

            key = cells[0];

            for (std::size_t c = 1; c < 6; ++c)
            {
                key += '\t';
                key += cells[c];
            }

            if (sec == 0)
            {
                if (bandOpen)
                {
                    ++shapeBad;
                    problem (strf ("line %lld: the band before it is incomplete", lineNumber));
                }

                if (! sameBits (fsCell, preparedFs))
                {
                    designer.prepare (fsCell);
                    grid = makeGrid (fsCell, 1000, 20.0, 20000.0, 0.499);
                    preparedFs = fsCell;
                }

                designer.design (mk (static_cast<eq::Type> (typeIndex), fcCell, gainCell, qCell, slope), current);
                golden = current;
                headKey = key;
                bandOpen = true;
                nextSec = 0;
            }

            if (! bandOpen || key != headKey || sec != nextSec || sec >= current.numSections)
            {
                ++shapeBad;
                bandOpen = false;
                problem (where() + ": section out of sequence");
                continue;
            }

            const eq::SectionParams& p = current.params[ix (sec)];
            const double values[8] = { p.g, p.k, p.mL, p.mB, p.mH, p.G, p.cL, p.cH };
            double goldValues[8] = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 };
            bool shapeOk = cells[7] == kClassNames[static_cast<std::size_t> (current.cls[ix (sec)])]
                        && cells[8] == (p.hasSvf ? "1" : "0")
                        && cells[9] == (p.hasOnePole ? "1" : "0");

            for (int c = 0; c < 8; ++c)
            {
                const std::string& cell = cells[ix (10 + c)];
                const bool used = c < 5 ? p.hasSvf : p.hasOnePole;
                double goldValue = 0.0;

                if (! used)
                {
                    shapeOk = shapeOk && cell == "-";
                    continue;
                }

                if (! parseDouble (cell, goldValue) || ! std::isfinite (goldValue))
                {
                    shapeOk = false;
                    continue;
                }

                goldValues[c] = goldValue;

                const double mine = values[c];
                const double tolerance = (c == 2 || c == 6) ? 1.0e-9 : 1.0e-6;      // mL, cL: copied from the seed
                ++cellsCompared;

                bool ok = std::isfinite (mine);

                if (ok && ! (std::fabs (goldValue) > 0.0))
                {
                    ok = std::fabs (mine) <= 1.0e-12;
                }
                else if (ok)
                {
                    const double rel = std::fabs (mine - goldValue) / std::fabs (goldValue);

                    if (rel > worstRel)
                    {
                        worstRel = rel;
                        worstWhere = where() + " column " + columnNames[c];
                    }

                    ok = rel <= tolerance;
                }

                if (! ok)
                {
                    ++cellsBad;
                    problem (where() + strf (" column %s: C++ %.17g, golden %.17g", columnNames[c], mine, goldValue));
                }
            }

            {
                const double mineWeight = static_cast<double> (current.polishWeight[ix (sec)]);
                const double difference = std::fabs (mineWeight - weightCell);

                if (! (difference <= worstWeight))
                    worstWeight = std::isfinite (difference) ? difference : kInf;

                if (! (difference <= 1.0e-6))
                {
                    ++weightBad;
                    problem (where() + strf (" weight: C++ %.9g, golden %.9g", mineWeight, weightCell));
                }
            }

            if (! shapeOk)
            {
                ++shapeBad;
                problem (where() + ": class / flag / unused-cell mismatch");
            }

            eq::SectionParams& gp = golden.params[ix (sec)];

            if (p.hasSvf)
            {
                gp.g = goldValues[0];
                gp.k = goldValues[1];
                gp.mL = goldValues[2];
                gp.mB = goldValues[3];
                gp.mH = goldValues[4];
            }

            if (p.hasOnePole)
            {
                gp.G = goldValues[5];
                gp.cL = goldValues[6];
                gp.cH = goldValues[7];
            }

            if (++nextSec == current.numSections)
            {
                // The whole band: the golden's parameters against the C++ design on the
                // evaluation grid (1000 log points + the points tied to every section).
                bandOpen = false;
                ++bands;
                double bandWorst = 0.0;

                const auto point = [&] (double t2)
                {
                    const double a = bandDigitalMag2 (current, t2);
                    const double b = bandDigitalMag2 (golden, t2);
                    const double e = std::fabs (10.0 * std::log10 ((a < kTiny ? kTiny : a) / (b < kTiny ? kTiny : b)));

                    if (! (e <= bandWorst))
                        bandWorst = std::isfinite (e) ? e : kInf;
                };

                for (const double t2 : grid.t2)
                    point (t2);

                collectTied (current, grid.wLo, grid.wHi, tied);

                for (int i = 0; i < tied.n; ++i)
                    point (tied.t2[ix (i)]);

                if (bandWorst > worstResponse)
                {
                    worstResponse = bandWorst;
                    worstResponseWhere = where();
                }

                if (! (bandWorst <= 1.0e-6))
                {
                    ++bandsBad;
                    problem (where() + strf (": band response differs by %.3g dB", bandWorst));
                }
            }
        }

        if (bandOpen)
        {
            ++shapeBad;
            problem ("the golden ends inside a band");
        }

        const bool ok = dataLines > 0 && dataLines == expectedLines && parseBad == 0 && shapeBad == 0
                     && cellsCompared > 0 && cellsBad == 0 && weightBad == 0 && bandsBad == 0 && bands > 0;

        row (ok, name,
             strf ("%lld data lines (C++ list: %lld), %lld unparsed, %lld shape; %lld cells, %lld out of tolerance, worst rel %.3g; "
                   "weights: %lld off, worst %.3g; %lld bands, %lld off, worst %.3g dB",
                   dataLines, expectedLines, parseBad, shapeBad, cellsCompared, cellsBad, worstRel,
                   weightBad, worstWeight, bands, bandsBad, worstResponse));

        if (! worstWhere.empty())
            note ("worst cell: " + worstWhere);

        if (! worstResponseWhere.empty())
            note ("worst band response ends at: " + worstResponseWhere);

        if (! firstProblem.empty())
            note ("first problem: " + firstProblem);
    }

    //==========================================================================
    // NW1: never worse, per section
    //==========================================================================
    /** The settings of one G1b family (rate-independent). */
    std::vector<eq::BandSettings> g1bSettings (G1Kind kind)
    {
        static const int steepIndex[5] = { 3, 4, 5, 6, 7 };
        const std::vector<double> fcs = geomspace (20.0, 20000.0, 40);
        const std::vector<double> qs = g1Qs (kind);
        std::vector<eq::BandSettings> list;

        for (const double gainAbs : kG1Gains)
            for (const double sign : { 1.0, -1.0 })
                for (const double fc : fcs)
                    for (const double q : qs)
                    {
                        if (kind == G1Kind::bell)
                        {
                            list.push_back (mk (eq::Type::bell, fc, sign * gainAbs, q, 1));
                            continue;
                        }

                        for (const eq::Type type : { eq::Type::lowShelf, eq::Type::highShelf })
                        {
                            if (kind == G1Kind::shelfSteep)
                            {
                                for (const int si : steepIndex)
                                    list.push_back (mk (type, fc, sign * gainAbs, q, si));
                            }
                            else
                            {
                                list.push_back (mk (type, fc, sign * gainAbs, q, kind == G1Kind::shelf6 ? 0 : 1));
                            }
                        }
                    }

        return list;
    }

    /** Largest |digital - analog| in dB of ONE gain2 section against its own
        true analog section (the reciprocal for a cut), on the grid plus the
        points tied to that section. +infinity for any non-finite value. */
    double sectionMaxError (const eq::SectionParams& p, const eq::AnalogSection& a, bool inverted, const Grid& grid)
    {
        Tied tied;
        addTiedPair (tied, a.wz, a.Qz, grid.wLo, grid.wHi);
        addTiedPair (tied, a.wp, a.Qp, grid.wLo, grid.wHi);

        double worstRatio = 1.0;
        bool bad = false;

        const auto point = [&] (double w, double t2)
        {
            const double analog = eq::analogMag2 (a, eq::SectionClass::gain2, w);
            const double target = inverted ? 1.0 / analog : analog;
            const double r = eq::sectionMag2 (t2, p) / target;
            const double rr = r > 1.0 ? r : 1.0 / r;

            if (! std::isfinite (rr) || ! (rr >= 1.0))
                bad = true;
            else if (rr > worstRatio)
                worstRatio = rr;
        };

        for (std::size_t i = 0; i < grid.w.size(); ++i)
            point (grid.w[i], grid.t2[i]);

        for (int i = 0; i < tied.n; ++i)
            point (tied.w[ix (i)], tied.t2[ix (i)]);

        return bad ? kInf : 10.0 * std::log10 (worstRatio);
    }

    void rowNW1()
    {
        struct RateResult
        {
            long long sections = 0, violations = 0, nonFinite = 0;
            double worstDelta = -1.0e300;
            std::string worstWhere;
        };

        std::vector<eq::BandSettings> settings;

        for (const G1Kind kind : { G1Kind::bell, G1Kind::shelf12Wide, G1Kind::shelfSteep })
        {
            const std::vector<eq::BandSettings> family = g1bSettings (kind);
            settings.insert (settings.end(), family.begin(), family.end());
        }

        std::array<RateResult, 5> results;

        parallelFor (5, [&settings, &results] (int r)
        {
            const double fs = kRates5[ix (r)];
            const Grid grid = makeGrid (fs, 1000, 20.0, 20000.0, 0.499);
            eq::Designer designer;
            designer.prepare (fs);
            eq::BandDesign seedDesign {}, polished {};
            RateResult& out = results[ix (r)];

            for (const eq::BandSettings& s : settings)
            {
                designer.designSeed (s, seedDesign);
                designer.design (s, polished);

                for (int i = 0; i < polished.numSections; ++i)
                {
                    if (polished.cls[ix (i)] != eq::SectionClass::gain2)
                        continue;

                    const double eSeed = sectionMaxError (seedDesign.params[ix (i)], seedDesign.trueProto[ix (i)],
                                                          seedDesign.inverted, grid);
                    const double ePolished = sectionMaxError (polished.params[ix (i)], polished.trueProto[ix (i)],
                                                              polished.inverted, grid);
                    ++out.sections;

                    if (! std::isfinite (eSeed) || ! std::isfinite (ePolished))
                    {
                        ++out.nonFinite;
                        continue;
                    }

                    const double delta = ePolished - eSeed;

                    if (delta > 1.0e-3)
                        ++out.violations;

                    if (delta > out.worstDelta)
                    {
                        out.worstDelta = delta;
                        out.worstWhere = describeSettings (s)
                                           + strf (" fs %.0f section %d: seed %.5f, polished %.5f dB", fs, i, eSeed, ePolished);
                    }
                }
            }
        });

        long long sections = 0, violations = 0, nonFinite = 0;
        double worstDelta = -1.0e300;
        std::string worstWhere;

        for (const RateResult& r : results)
        {
            sections += r.sections;
            violations += r.violations;
            nonFinite += r.nonFinite;

            if (r.worstDelta > worstDelta)
            {
                worstDelta = r.worstDelta;
                worstWhere = r.worstWhere;
            }
        }

        row (violations == 0 && nonFinite == 0 && sections > 0,
             "NW1 per gain2 section: polished max error <= seed max error + 1e-3 dB (G1b grids, five rates)",
             strf ("%lld violations, %lld non-finite, %lld sections compared; worst (polished - seed) %+.3g dB (model: 0 violations)",
                   violations, nonFinite, sections, worstDelta));

        if (! worstWhere.empty())
            note ("worst (polished - seed) at: " + worstWhere);
    }

    //==========================================================================
    // C1 / C1n: continuity on lines through parameter space
    //==========================================================================
    /** The control of C1n: the shipped policy with the extension weight as a
        binary gate (RESEARCH-2.1 Q4). It differs from the shipped one in the
        gate only. */
    struct BinaryExtension
    {
        static double weight (double x) noexcept            { return x > 0.0 ? 1.0 : 0.0; }
        static constexpr bool limitSteps = true;
        static constexpr double stepLimit = eq::polish::stepLimit;
    };

    /** The polish of the first-build control: the smooth weight, no step limit. */
    struct FirstBuildPolicy
    {
        static double weight (double x) noexcept            { return eq::detail::smoothStep (x); }
        static constexpr bool limitSteps = false;
        [[maybe_unused]] static constexpr double stepLimit = 0.0;   // never read: the limit is compiled out
    };

    /** The first-build control (--neg first-build): a shelf / tilt second-order
        section is seeded from its TRUE section (no shelf re-centre) and, when
        `polish` is set, polished without a step limit. A bell keeps its
        re-centred design side as the seed, the first-order shelf section is
        the closed form, and the tilt scalar and the inversion follow as in
        the designer. */
    void designFirstBuild (const eq::Designer& designer, bool polish, const eq::BandSettings& s, eq::BandDesign& out)
    {
        designer.designSeed (s, out);       // counts, classes, true prototypes, the inverted flag; weights 0

        if (! eq::isGainType (s.type))
            return;

        eq::AnalogBand proto;
        eq::buildPrototype (s, designer.getSampleRate(), proto);

        for (int i = 0; i < proto.numSections; ++i)
        {
            const std::size_t u = ix (i);

            if (proto.cls[u] != eq::SectionClass::gain2)
            {
                out.params[u] = eq::firstOrderShelf (proto.wc, proto.shelfGain[u], proto.shelfHigh[u]);
                continue;
            }

            const eq::AnalogSection& start = s.type == eq::Type::bell ? proto.designSide[u] : proto.trueSide[u];
            out.params[u] = eq::Designer::designSeedSection (start);

            if (polish)
            {
                eq::PolishInfo polishInfo { 0.0, 0 };
                out.params[u] = eq::detail::polishSectionWith<FirstBuildPolicy> (designer.getPolishGrid(), proto.trueSide[u],
                                                                                 out.params[u], &polishInfo);
                out.polishWeight[u] = static_cast<float> (polishInfo.weight);
            }
        }

        if (s.type == eq::Type::tilt && proto.numSections > 0)
        {
            eq::SectionParams& last = out.params[ix (proto.numSections - 1)];
            last.cL = last.cL * proto.tiltScalar;
            last.cH = last.cH * proto.tiltScalar;
        }

        if (out.inverted)
            for (int i = 0; i < proto.numSections; ++i)
                eq::invert (out.params[ix (i)]);
    }

    /** What a continuity or never-worse row scores: a seed and the design
        built on it. */
    template <typename Policy>
    struct PolicyVariant
    {
        static void seed (const eq::Designer& designer, const eq::BandSettings& s, eq::BandDesign& out)
        {
            designer.designSeed (s, out);
        }

        static void design (const eq::Designer& designer, const eq::BandSettings& s, eq::BandDesign& out)
        {
            designer.designWith<Policy> (s, out);
        }
    };

    using ShippedVariant = PolicyVariant<eq::detail::ShippedPolicy>;

    struct FirstBuildVariant
    {
        static void seed (const eq::Designer& designer, const eq::BandSettings& s, eq::BandDesign& out)
        {
            designFirstBuild (designer, false, s, out);
        }

        static void design (const eq::Designer& designer, const eq::BandSettings& s, eq::BandDesign& out)
        {
            designFirstBuild (designer, true, s, out);
        }
    };

    struct C1Line
    {
        const char* family;
        int index;
        bool firstStageOfC1n;       // the fixed sweeps and the random shelf lines
        eq::Type type;
        int slopeIndex;
        double fs;
        double fc0, fc1, gain0, gain1, q0, q1;
    };

    std::string c1Describe (const C1Line& line)
    {
        return strf ("%s #%d (%s, slope-index %d, fs %.0f): fc %.9g -> %.9g Hz, gain %.9g -> %.9g dB, Q %.9g -> %.9g",
                     line.family, line.index, kTypeNames[static_cast<int> (line.type)], line.slopeIndex, line.fs,
                     line.fc0, line.fc1, line.gain0, line.gain1, line.q0, line.q1);
    }

    /** Block A: 320 seeded random lines (40 per family and rate), then the 21
        fixed one-dimensional sweeps of research-2.1/t10_cont.py at 44.1 kHz. */
    std::vector<C1Line> c1Lines()
    {
        using T = eq::Type;
        std::vector<C1Line> lines;

        struct Family { const char* name; int slopeIndex; int mode; bool shelf; };     // mode 0 bell, 1 low / high shelf, 2 tilt
        static const Family families[4] =
        {
            { "random bell",             1, 0, false },
            { "random shelf 12 dB/oct",  1, 1, true  },
            { "random shelf 48 dB/oct",  5, 1, true  },
            { "random tilt 12 dB/oct",   1, 2, false }
        };

        Rng rng (0xC1C1C1C1ull);

        for (const Family& family : families)
            for (const double fs : { 44100.0, 48000.0 })
                for (int k = 0; k < 40; ++k)
                {
                    C1Line line {};
                    line.family = family.name;
                    line.index = k;
                    line.firstStageOfC1n = family.shelf;
                    line.type = family.mode == 0 ? T::bell
                                                 : (family.mode == 2 ? T::tilt : (k % 2 == 0 ? T::lowShelf : T::highShelf));
                    line.slopeIndex = family.slopeIndex;
                    line.fs = fs;
                    line.fc0 = 10.0 * std::pow (3000.0, rng.uniform());
                    line.gain0 = -60.0 + 120.0 * rng.uniform();
                    line.q0 = 0.025 * std::pow (1600.0, rng.uniform());
                    line.fc1 = 10.0 * std::pow (3000.0, rng.uniform());
                    line.gain1 = -60.0 + 120.0 * rng.uniform();
                    line.q1 = 0.025 * std::pow (1600.0, rng.uniform());
                    lines.push_back (line);
                }

        // kind: 0 bell, 1 low shelf 12 dB/oct, 2 high shelf 12 dB/oct, 3 high shelf 48 dB/oct (the whole band)
        const auto addFixed = [&lines] (const char* family, int index, int kind,
                                        double fc0, double fc1, double gain0, double gain1, double q0, double q1)
        {
            C1Line line {};
            line.family = family;
            line.index = index;
            line.firstStageOfC1n = true;
            line.type = kind == 0 ? T::bell : (kind == 1 ? T::lowShelf : T::highShelf);
            line.slopeIndex = kind == 3 ? 5 : 1;
            line.fs = 44100.0;
            line.fc0 = fc0;
            line.fc1 = fc1;
            line.gain0 = gain0;
            line.gain1 = gain1;
            line.q0 = q0;
            line.q1 = q1;
            lines.push_back (line);
        };

        struct Fixed { int kind; double a, b; };

        static const Fixed fcSweeps[10] =       // kind, Q, gain; fc 10 Hz -> 30 kHz
        {
            { 0, 1.0, 12.0 }, { 0, 0.1, 30.0 }, { 0, 10.0, 30.0 }, { 0, 40.0, 6.0 }, { 0, 0.025, 18.0 },
            { 1, 0.71, 12.0 }, { 2, 1.0, 30.0 }, { 2, 4.0, 12.0 }, { 1, 10.0, 6.0 }, { 3, 1.0, 30.0 }
        };

        static const Fixed gainSweeps[6] =      // kind, Q, fc; gain 0.001 -> 60 dB
        {
            { 0, 1.0, 15000.0 }, { 0, 0.2, 8000.0 }, { 0, 8.0, 19000.0 },
            { 2, 1.0, 12000.0 }, { 1, 3.0, 14000.0 }, { 0, 1.0, 25000.0 }
        };

        static const Fixed qSweeps[5] =         // kind, gain, fc; Q 0.025 -> 40
        {
            { 0, 12.0, 15000.0 }, { 0, 30.0, 5000.0 }, { 2, 12.0, 12000.0 }, { 1, 18.0, 16000.0 }, { 0, 18.0, 22000.0 }
        };

        int fixedIndex = 0;

        for (const Fixed& f : fcSweeps)
            addFixed ("fixed fc sweep", fixedIndex++, f.kind, 10.0, 30000.0, f.b, f.b, f.a, f.a);

        for (const Fixed& f : gainSweeps)
            addFixed ("fixed gain sweep", fixedIndex++, f.kind, f.b, f.b, 0.001, 60.0, f.a, f.a);

        for (const Fixed& f : qSweeps)
            addFixed ("fixed Q sweep", fixedIndex++, f.kind, f.b, f.b, f.a, f.a, 0.025, 40.0);

        return lines;
    }

    struct C1Family { const char* name; int slopeIndex; int mode; };       // mode 0 bell, 1 low / high shelf, 2 tilt

    /** `count` random lines of one family at one rate, drawn as block A draws them. */
    void c1AppendRandom (std::vector<C1Line>& lines, Rng& rng, const C1Family& family, double fs, int count)
    {
        using T = eq::Type;

        for (int k = 0; k < count; ++k)
        {
            C1Line line {};
            line.family = family.name;
            line.index = k;
            line.firstStageOfC1n = false;
            line.type = family.mode == 0 ? T::bell
                                         : (family.mode == 2 ? T::tilt : (k % 2 == 0 ? T::lowShelf : T::highShelf));
            line.slopeIndex = family.slopeIndex;
            line.fs = fs;
            line.fc0 = 10.0 * std::pow (3000.0, rng.uniform());
            line.gain0 = -60.0 + 120.0 * rng.uniform();
            line.q0 = 0.025 * std::pow (1600.0, rng.uniform());
            line.fc1 = 10.0 * std::pow (3000.0, rng.uniform());
            line.gain1 = -60.0 + 120.0 * rng.uniform();
            line.q1 = 0.025 * std::pow (1600.0, rng.uniform());
            lines.push_back (line);
        }
    }

    /** Block B, its own generator: 60 lines per rate for each of five further
        families, then 20 more per rate for each family of block A. 760 lines. */
    std::vector<C1Line> c1LinesBlockB()
    {
        static const C1Family added[5] =
        {
            { "random shelf 18 dB/oct", 2, 1 },
            { "random shelf 24 dB/oct", 3, 1 },
            { "random shelf 72 dB/oct", 6, 1 },
            { "random shelf 96 dB/oct", 7, 1 },
            { "random tilt 48 dB/oct",  5, 2 }
        };

        static const C1Family more[4] =
        {
            { "block B random bell",            1, 0 },
            { "block B random shelf 12 dB/oct", 1, 1 },
            { "block B random shelf 48 dB/oct", 5, 1 },
            { "block B random tilt 12 dB/oct",  1, 2 }
        };

        std::vector<C1Line> lines;
        Rng rng (0xC1C1C1C2ull);

        for (const C1Family& family : added)
            for (const double fs : { 44100.0, 48000.0 })
                c1AppendRandom (lines, rng, family, fs, 60);

        for (const C1Family& family : more)
            for (const double fs : { 44100.0, 48000.0 })
                c1AppendRandom (lines, rng, family, fs, 20);

        return lines;
    }

    /** Row C1's lines: block A (341), then block B (760). */
    std::vector<C1Line> c1AllLines()
    {
        std::vector<C1Line> lines = c1Lines();
        const std::vector<C1Line> blockB = c1LinesBlockB();
        lines.insert (lines.end(), blockB.begin(), blockB.end());
        return lines;
    }

    /** Row C1-hi's lines: nine families x 88.2 / 96 / 192 kHz x 8. 216 lines. */
    std::vector<C1Line> c1LinesHighRates()
    {
        static const C1Family families[9] =
        {
            { "high-rate bell",            1, 0 },
            { "high-rate shelf 12 dB/oct", 1, 1 },
            { "high-rate shelf 18 dB/oct", 2, 1 },
            { "high-rate shelf 24 dB/oct", 3, 1 },
            { "high-rate shelf 48 dB/oct", 5, 1 },
            { "high-rate shelf 72 dB/oct", 6, 1 },
            { "high-rate shelf 96 dB/oct", 7, 1 },
            { "high-rate tilt 12 dB/oct",  1, 2 },
            { "high-rate tilt 48 dB/oct",  5, 2 }
        };

        std::vector<C1Line> lines;
        Rng rng (0xC1C1C1C3ull);

        for (const C1Family& family : families)
            for (const double fs : { 88200.0, 96000.0, 192000.0 })
                c1AppendRandom (lines, rng, family, fs, 8);

        return lines;
    }

    /** Largest |dB change| between neighbouring steps, at 8000 and at 32000 steps. */
    struct C1Result
    {
        double seedJump[2] = { 0.0, 0.0 };
        double polishedJump[2] = { 0.0, 0.0 };
        bool bad = false;
    };

    /** One line. Settings move linearly in (log fc, gain dB, log Q); the
        response is compared between neighbouring steps on a FIXED grid of 160
        log points 20 Hz -> min (20 kHz, 0.499 fs). No allocation in the loop.
        Variant supplies the seed and the design built on it. */
    template <typename Variant>
    C1Result c1RunLine (const C1Line& line)
    {
        constexpr int numPoints = 160;
        static const int stepCounts[2] = { 8000, 32000 };

        std::array<double, numPoints> t2 {};
        const double fHi = std::min (20000.0, 0.499 * line.fs);

        for (int p = 0; p < numPoints; ++p)
        {
            const double f = 20.0 * std::pow (fHi / 20.0, static_cast<double> (p) / static_cast<double> (numPoints - 1));
            t2[ix (p)] = eq::tanSquaredHalf (2.0 * kPi * f / line.fs);
        }

        const double logFc0 = std::log (line.fc0), logFc1 = std::log (line.fc1);
        const double logQ0 = std::log (line.q0), logQ1 = std::log (line.q1);

        eq::Designer designer;
        designer.prepare (line.fs);
        eq::BandDesign seedDesign {}, polished {};
        std::array<double, numPoints> previousSeed {}, previousPolished {};
        C1Result result;

        for (int pass = 0; pass < 2; ++pass)
        {
            const int n = stepCounts[pass];
            double worstSeed = 1.0, worstPolished = 1.0;

            for (int i = 0; i < n; ++i)
            {
                const double x = static_cast<double> (i) / static_cast<double> (n - 1);
                const eq::BandSettings s = mk (line.type,
                                               std::exp (logFc0 + x * (logFc1 - logFc0)),
                                               line.gain0 + x * (line.gain1 - line.gain0),
                                               std::exp (logQ0 + x * (logQ1 - logQ0)),
                                               line.slopeIndex);

                Variant::seed (designer, s, seedDesign);
                Variant::design (designer, s, polished);

                for (int p = 0; p < numPoints; ++p)
                {
                    const double a = bandDigitalMag2 (seedDesign, t2[ix (p)]);
                    const double b = bandDigitalMag2 (polished, t2[ix (p)]);

                    if (i > 0)
                    {
                        const double pa = previousSeed[ix (p)];
                        const double pb = previousPolished[ix (p)];
                        const double ra = a > pa ? a / pa : pa / a;
                        const double rb = b > pb ? b / pb : pb / b;

                        if (! (ra >= 1.0) || ! (rb >= 1.0) || ! std::isfinite (ra) || ! std::isfinite (rb))
                            result.bad = true;

                        if (ra > worstSeed)
                            worstSeed = ra;

                        if (rb > worstPolished)
                            worstPolished = rb;
                    }

                    previousSeed[ix (p)] = a;
                    previousPolished[ix (p)] = b;
                }
            }

            result.seedJump[pass] = 10.0 * std::log10 (worstSeed);
            result.polishedJump[pass] = 10.0 * std::log10 (worstPolished);
        }

        return result;
    }

    /** Every line on worker threads, results stored per line. */
    template <typename Variant>
    std::vector<C1Result> c1RunLines (const std::vector<C1Line>& lines)
    {
        std::vector<C1Result> results (lines.size());

        parallelFor (static_cast<int> (lines.size()), [&lines, &results] (int i)
        {
            results[ix (i)] = c1RunLine<Variant> (lines[ix (i)]);
        });

        return results;
    }

    constexpr double kC1RatioLimit = 4.0;       // polished jump / seed jump, both step counts
    constexpr double kC1ShrinkLimit = 0.5;      // polished jump (32000) / polished jump (8000)
    constexpr double kC1FloorDb = 1.0e-9;

    /** One line against the two limits. A jump below 1e-9 dB counts as 1e-9,
        and a 32000-step jump at or below that floor cannot fail the second
        test. `ratio` is the larger of the two step counts' ratios. */
    struct C1Verdict
    {
        double ratio = 0.0, shrink = 0.0;
        bool violates = false;
    };

    C1Verdict c1Judge (const C1Result& r)
    {
        C1Verdict v;

        if (r.bad)
        {
            v.violates = true;
            return v;
        }

        for (int pass = 0; pass < 2; ++pass)
            v.ratio = std::max (v.ratio, std::max (r.polishedJump[pass], kC1FloorDb) / std::max (r.seedJump[pass], kC1FloorDb));

        v.shrink = r.polishedJump[1] > kC1FloorDb
                     ? std::max (r.polishedJump[1], kC1FloorDb) / std::max (r.polishedJump[0], kC1FloorDb)
                     : 0.0;
        v.violates = v.ratio > kC1RatioLimit || v.shrink > kC1ShrinkLimit;
        return v;
    }

    struct C1Summary
    {
        int lines = 0, violations = 0, nonFinite = 0, notShrinking = 0, above2x = 0;
        double worstVsSeed = 0.0, worstShrink = 0.0;
        std::string worstVsSeedLine, worstShrinkLine, firstViolation;
    };

    /** Adds the lines and their results to `sum`. */
    void c1Accumulate (const std::vector<C1Line>& lines, const std::vector<C1Result>& results, C1Summary& sum)
    {
        for (std::size_t i = 0; i < lines.size(); ++i)
        {
            const C1Result& r = results[i];
            const C1Verdict v = c1Judge (r);
            ++sum.lines;

            if (r.bad)
            {
                ++sum.nonFinite;
                ++sum.violations;

                if (sum.firstViolation.empty())
                    sum.firstViolation = c1Describe (lines[i]) + ": NON-FINITE or zero response";

                continue;
            }

            if (v.ratio > sum.worstVsSeed)
            {
                sum.worstVsSeed = v.ratio;
                sum.worstVsSeedLine = c1Describe (lines[i]);
            }

            if (v.ratio > 2.0)
                ++sum.above2x;

            if (v.shrink > sum.worstShrink)
            {
                sum.worstShrink = v.shrink;
                sum.worstShrinkLine = c1Describe (lines[i]);
            }

            if (v.shrink > kC1ShrinkLimit)
                ++sum.notShrinking;

            if (v.violates)
            {
                ++sum.violations;

                if (sum.firstViolation.empty())
                    sum.firstViolation = c1Describe (lines[i])
                                           + strf (": seed %.6f / %.6f dB, polished %.6f / %.6f dB at 8000 / 32000 steps",
                                                   r.seedJump[0], r.seedJump[1], r.polishedJump[0], r.polishedJump[1]);
            }
        }
    }

    std::string c1Detail (const C1Summary& sum)
    {
        return strf ("%d lines, %d violations (%d non-finite, %d not shrinking); worst polished / seed jump %.3f (limit %g), "
                     "%d lines above 2x; worst jump (32000) / jump (8000) %.3f (limit %g)",
                     sum.lines, sum.violations, sum.nonFinite, sum.notShrinking, sum.worstVsSeed, kC1RatioLimit,
                     sum.above2x, sum.worstShrink, kC1ShrinkLimit);
    }

    /** The figures of one named line, whatever its verdict. */
    void c1NoteNamed (const std::vector<C1Line>& lines, const std::vector<C1Result>& results,
                      const char* family, int index, double fs)
    {
        for (std::size_t i = 0; i < lines.size(); ++i)
        {
            const C1Line& line = lines[i];

            if (std::strcmp (line.family, family) != 0 || line.index != index || ! sameBits (line.fs, fs))
                continue;

            const C1Result& r = results[i];
            const C1Verdict v = c1Judge (r);
            note (strf ("failed in the first build: %s #%d at %.0f Hz: seed %.6f / %.6f dB, polished %.6f / %.6f dB at 8000 / 32000 steps; "
                        "polished / seed %.3f, 32000 / 8000 %.3f: %s",
                        family, index, fs, r.seedJump[0], r.seedJump[1], r.polishedJump[0], r.polishedJump[1],
                        v.ratio, v.shrink, r.bad ? "NON-FINITE" : (v.violates ? "VIOLATES" : "within both limits")));
            return;
        }

        note (strf ("failed in the first build: %s #%d at %.0f Hz: NOT in the line set", family, index, fs));
    }

    void c1Notes (const C1Summary& sum)
    {
        if (! sum.worstVsSeedLine.empty())
            note ("worst polished / seed: " + sum.worstVsSeedLine);

        if (! sum.worstShrinkLine.empty())
            note ("worst 32000 / 8000:    " + sum.worstShrinkLine);

        if (! sum.firstViolation.empty())
            note ("first violation:       " + sum.firstViolation);
    }

    /** Returns, per line of c1AllLines(), whether the shipped design passes it. */
    std::vector<bool> rowC1()
    {
        const std::vector<C1Line> lines = c1AllLines();
        const std::vector<C1Result> results = c1RunLines<ShippedVariant> (lines);
        C1Summary sum;
        c1Accumulate (lines, results, sum);

        row (sum.violations == 0 && sum.lines == 1101,
             "C1 continuity: block A (320 random lines + 21 fixed sweeps) + block B (760 random lines), 8000 and 32000 steps",
             c1Detail (sum) + "; research: about 1 line in 1000 above 2x, at most 2.4x, shrinking 4x");
        c1Notes (sum);
        c1NoteNamed (lines, results, "random shelf 48 dB/oct", 27, 48000.0);
        c1NoteNamed (lines, results, "random shelf 48 dB/oct", 2, 44100.0);

        std::vector<bool> shippedPass (lines.size());

        for (std::size_t i = 0; i < lines.size(); ++i)
            shippedPass[i] = ! c1Judge (results[i]).violates;

        return shippedPass;
    }

    void rowC1Hi()
    {
        const std::vector<C1Line> lines = c1LinesHighRates();
        const std::vector<C1Result> results = c1RunLines<ShippedVariant> (lines);
        C1Summary sum;
        c1Accumulate (lines, results, sum);

        row (sum.violations == 0 && sum.lines == 216,
             "C1-hi continuity at 88.2 / 96 / 192 kHz: 216 random lines (9 families x 3 rates x 8), 8000 and 32000 steps",
             c1Detail (sum) + "; research (step limit 0.5 only): 0 of 1080");
        c1Notes (sum);
    }

    /** C1 with the binary extension gate, scored only on lines the shipped
        design passes. First the fixed sweeps and the random shelf lines of
        block A; the remaining lines only if none of those fails.
        `shippedPass` is rowC1's result; when it is null the shipped design is
        run here on the same lines. `excluded` counts the lines left out. */
    bool c1BinaryGateFires (const std::vector<bool>* shippedPass, C1Summary& sum, bool& ranSecondStage, int& excluded)
    {
        const std::vector<C1Line> lines = c1AllLines();
        ranSecondStage = false;
        excluded = 0;

        for (int stage = 1; stage <= 2; ++stage)
        {
            std::vector<C1Line> candidates;
            std::vector<bool> candidatePass;

            for (std::size_t i = 0; i < lines.size(); ++i)
                if ((stage == 1) == lines[i].firstStageOfC1n)
                {
                    candidates.push_back (lines[i]);
                    candidatePass.push_back (shippedPass == nullptr || i >= shippedPass->size() || (*shippedPass)[i]);
                }

            if (shippedPass == nullptr)
            {
                const std::vector<C1Result> shipped = c1RunLines<ShippedVariant> (candidates);

                for (std::size_t j = 0; j < candidates.size(); ++j)
                    candidatePass[j] = ! c1Judge (shipped[j]).violates;
            }

            std::vector<C1Line> scored;

            for (std::size_t j = 0; j < candidates.size(); ++j)
            {
                if (candidatePass[j])
                    scored.push_back (candidates[j]);
                else
                    ++excluded;
            }

            const std::vector<C1Result> results = c1RunLines<PolicyVariant<BinaryExtension>> (scored);
            c1Accumulate (scored, results, sum);

            if (sum.violations > 0)
                return true;

            if (stage == 1)
                ranSecondStage = true;
        }

        return false;
    }

    std::string c1BinaryGateScope (bool ranSecondStage, int excluded)
    {
        return std::string (ranSecondStage ? "the fixed sweeps and block A shelf lines did NOT fail, every line was run"
                                           : "fixed sweeps + block A random shelf lines")
                 + strf ("; %d lines the shipped design fails left out", excluded);
    }

    void rowC1n (const std::vector<bool>& shippedPass)
    {
        C1Summary sum;
        bool ranSecondStage = false;
        int excluded = 0;
        const bool fires = c1BinaryGateFires (&shippedPass, sum, ranSecondStage, excluded);

        row (fires, "C1n the C1 test FAILS a binary extension gate (designWith<BinaryExtension>) on a line the shipped design passes",
             c1Detail (sum) + "; " + c1BinaryGateScope (ranSecondStage, excluded));
        c1Notes (sum);
    }

    //==========================================================================
    // NW2 / RA1: never far worse than the seed, and no runaway fit, on a
    // random full-range draw
    //==========================================================================
    constexpr int kNw2BandsPerType = 250000;
    constexpr long long kNw2MinSections = 1000000;
    constexpr double kNw2CeilingDb = 1.0;

    /** Region 0: section Q >= 4 with its upper pair above 0.9 Nyquist. Region 1: the rest. */
    struct Nw2Result
    {
        long long bands = 0, sections = 0, weighted = 0, nonFinite = 0, over = 0, runaway = 0;
        long long regionSections[2] = { 0, 0 }, regionAbove[2] = { 0, 0 };
        double regionWorst[2] = { 0.0, 0.0 };
        double worst = -1.0e300;
        std::string worstWhere, firstRunaway;
    };

    /** 250 000 bands per type (Bell, Low Shelf, High Shelf, Tilt) at one rate:
        slope uniform, fc 10 Hz - 30 kHz log, gain 0 - 60 dB, Q 0.025 - 40 log.
        Every gain2 section is scored against its own true prototype on 400
        log points over the match band plus its tied points: polished max
        error - seed max error. A section with weight > 0 whose g or k is more
        than 10x from the seed's, either way, is a runaway.

        Fixed chunks, each with its own generator, merged in chunk order: the
        result does not depend on the thread count. The draw is the same at
        every rate. */
    template <typename Variant>
    Nw2Result runNw2 (double fs)
    {
        constexpr int chunkSize = 2000;
        static const eq::Type types[4] = { eq::Type::bell, eq::Type::lowShelf, eq::Type::highShelf, eq::Type::tilt };

        const int total = 4 * kNw2BandsPerType;
        const int numChunks = (total + chunkSize - 1) / chunkSize;
        const Grid grid = makeGrid (fs, 400, 20.0, 20000.0, 0.499);
        const double tenfold = std::log (10.0);
        std::vector<Nw2Result> chunks (ix (numChunks));

        parallelFor (numChunks, [&chunks, &grid, fs, total, tenfold] (int c)
        {
            Rng chunkSeed (0x4E573200ull + static_cast<std::uint64_t> (c));
            Rng rng (chunkSeed.next());
            eq::Designer designer;
            designer.prepare (fs);
            eq::BandDesign seedDesign {}, polished {};
            Nw2Result& out = chunks[ix (c)];
            const int end = std::min (total, (c + 1) * chunkSize);

            for (int n = c * chunkSize; n < end; ++n)
            {
                const int slope = rng.below (8);
                const double fc = 10.0 * std::pow (3000.0, rng.uniform());
                const double gain = 60.0 * rng.uniform();
                const double q = 0.025 * std::pow (1600.0, rng.uniform());
                const eq::BandSettings s = mk (types[n % 4], fc, gain, q, slope);

                Variant::seed (designer, s, seedDesign);
                Variant::design (designer, s, polished);
                ++out.bands;

                for (int i = 0; i < polished.numSections; ++i)
                {
                    if (polished.cls[ix (i)] != eq::SectionClass::gain2)
                        continue;

                    const eq::AnalogSection& a = seedDesign.trueProto[ix (i)];
                    const eq::SectionParams& ps = seedDesign.params[ix (i)];
                    const eq::SectionParams& pp = polished.params[ix (i)];
                    ++out.sections;

                    if (polished.polishWeight[ix (i)] > 0.0f)
                    {
                        ++out.weighted;
                        const double moveG = std::fabs (std::log (pp.g / ps.g));
                        const double moveK = std::fabs (std::log (pp.k / ps.k));

                        if (! (moveG <= tenfold && moveK <= tenfold))
                        {
                            ++out.runaway;

                            if (out.firstRunaway.empty())
                                out.firstRunaway = describeSettings (s)
                                                     + strf (" fs %.0f section %d: g x %.3g, k x %.3g", fs, i, pp.g / ps.g, pp.k / ps.k);
                        }
                    }

                    const double eSeed = sectionMaxError (ps, a, false, grid);
                    const double ePolished = sectionMaxError (pp, a, false, grid);

                    if (! std::isfinite (eSeed) || ! std::isfinite (ePolished))
                    {
                        ++out.nonFinite;
                        continue;
                    }

                    const double delta = ePolished - eSeed;
                    const double upper = (a.wz > a.wp ? a.wz : a.wp) / kPi;
                    const int region = (a.Qp >= 4.0 && upper > 0.9) ? 0 : 1;
                    ++out.regionSections[region];

                    if (delta > 0.01)
                        ++out.regionAbove[region];

                    if (delta > out.regionWorst[region])
                        out.regionWorst[region] = delta;

                    if (delta > kNw2CeilingDb)
                        ++out.over;

                    if (delta > out.worst)
                    {
                        out.worst = delta;
                        out.worstWhere = describeSettings (s)
                                           + strf (" fs %.0f section %d: seed %.4f, polished %.4f dB", fs, i, eSeed, ePolished);
                    }
                }
            }
        });

        Nw2Result sum;

        for (const Nw2Result& r : chunks)
        {
            sum.bands += r.bands;
            sum.sections += r.sections;
            sum.weighted += r.weighted;
            sum.nonFinite += r.nonFinite;
            sum.over += r.over;
            sum.runaway += r.runaway;

            for (int region = 0; region < 2; ++region)
            {
                sum.regionSections[region] += r.regionSections[region];
                sum.regionAbove[region] += r.regionAbove[region];
                sum.regionWorst[region] = std::max (sum.regionWorst[region], r.regionWorst[region]);
            }

            if (r.worst > sum.worst)
            {
                sum.worst = r.worst;
                sum.worstWhere = r.worstWhere;
            }

            if (sum.firstRunaway.empty())
                sum.firstRunaway = r.firstRunaway;
        }

        return sum;
    }

    bool nw2Void (const Nw2Result& r)
    {
        return r.sections < kNw2MinSections;
    }

    bool nw2Passes (const Nw2Result& r)
    {
        return ! nw2Void (r) && r.nonFinite == 0 && r.over == 0;
    }

    std::string nw2Text (double fs, const Nw2Result& r)
    {
        return strf ("%.1f k: %lld bands, %lld gain2 sections%s, %.1f %% with weight > 0, %lld non-finite; "
                     "worst (polished - seed) %+.3f dB, %lld above +%g dB; above +0.01 dB: %lld of %lld with Q >= 4 and the upper pair "
                     "above 0.9 Nyquist (worst %+.3f), %lld of %lld in the rest (worst %+.3f); runaway %lld",
                     fs / 1000.0, r.bands, r.sections, nw2Void (r) ? " (VOID: under 1 000 000)" : "",
                     100.0 * static_cast<double> (r.weighted) / static_cast<double> (std::max (r.sections, 1LL)),
                     r.nonFinite, r.worst > -1.0e299 ? r.worst : 0.0, r.over, kNw2CeilingDb,
                     r.regionAbove[0], r.regionSections[0], r.regionWorst[0],
                     r.regionAbove[1], r.regionSections[1], r.regionWorst[1], r.runaway);
    }

    void nw2Notes (double fs, const Nw2Result& r)
    {
        note (nw2Text (fs, r));

        if (! r.worstWhere.empty())
            note ("  worst (polished - seed) at: " + r.worstWhere);

        if (! r.firstRunaway.empty())
            note ("  first runaway: " + r.firstRunaway);
    }

    void rowsNW2RA1()
    {
        static const double rates[2] = { 44100.0, 48000.0 };
        Nw2Result results[2];
        bool ok = true;
        long long runaway = 0, weighted = 0;

        for (int r = 0; r < 2; ++r)
        {
            results[r] = runNw2<ShippedVariant> (rates[r]);
            ok = ok && nw2Passes (results[r]);
            runaway += results[r].runaway;
            weighted += results[r].weighted;
        }

        row (ok, "NW2 random full range, per gain2 section: polished max error <= seed max error + 1.0 dB (44.1 / 48 kHz)",
             strf ("worst (polished - seed) %+.3f dB at 44.1 kHz, %+.3f dB at 48 kHz; %lld / %lld sections "
                   "(research: +0.67 dB over 2.8 million sections at 44.1 kHz)",
                   results[0].worst > -1.0e299 ? results[0].worst : 0.0, results[1].worst > -1.0e299 ? results[1].worst : 0.0,
                   results[0].sections, results[1].sections));

        for (int r = 0; r < 2; ++r)
            nw2Notes (rates[r], results[r]);

        row (runaway == 0 && weighted > 0,
             "RA1 on NW2's draw (44.1 / 48 kHz): no section with weight > 0 has g or k more than 10x from the seed's",
             strf ("%lld runaway of %lld sections with weight > 0 (research: 0; the first build about 1000 per rate)", runaway, weighted));

        const Nw2Result high = runNw2<ShippedVariant> (96000.0);
        info ("NW2 / RA1 on the same draw at 96 kHz (not gated)");
        nw2Notes (96000.0, high);
    }

    //==========================================================================
    // PR1: no polish below 44.1 kHz
    //==========================================================================
    void rowPR1()
    {
        constexpr int numSettings = 10000;
        std::vector<eq::BandSettings> settings;
        settings.reserve (ix (numSettings));
        Rng rng (0x50523100ull);

        for (int i = 0; i < numSettings; ++i)
            settings.push_back (randomSettings (rng));

        eq::BandDesign polished {}, seedDesign {};
        long long designs = 0, different = 0, weightBad = 0;
        bool gateOff = true;
        std::string first;

        for (const double fs : { 22050.0, 32000.0 })
        {
            eq::Designer designer;
            designer.prepare (fs);
            gateOff = gateOff && ! designer.isPolishEnabled();

            for (const eq::BandSettings& s : settings)
            {
                designer.design (s, polished);
                designer.designSeed (s, seedDesign);
                ++designs;

                bool weightsZero = true;

                for (int i = 0; i < eq::maxSections; ++i)
                    weightsZero = weightsZero && polished.polishWeight[ix (i)] == 0.0f;

                if (! weightsZero)
                    ++weightBad;

                if (! sameDesign (polished, seedDesign, true))
                {
                    ++different;

                    if (first.empty())
                        first = describeSettings (s) + strf (" fs %.0f", fs);
                }
            }
        }

        // Vacuity: the same settings do get polished at 44.1 kHz.
        eq::Designer reference;
        reference.prepare (44100.0);
        int polishedBands = 0;

        for (const eq::BandSettings& s : settings)
        {
            reference.design (s, polished);
            bool any = false;

            for (int i = 0; i < polished.numSections; ++i)
                any = any || polished.polishWeight[ix (i)] > 0.0f;

            if (any)
                ++polishedBands;
        }

        row (different == 0 && weightBad == 0 && gateOff && designs == 2LL * numSettings && polishedBands >= 1000,
             "PR1 no polish below 44.1 kHz: 10 000 mixed settings at 22.05 and 32 kHz, design == designSeed (memcmp), every weight 0",
             strf ("%lld designs, %lld differ, %lld with a weight other than 0, isPolishEnabled %s; "
                   "the same settings at 44.1 kHz: %d bands with a weight > 0%s",
                   designs, different, weightBad, gateOff ? "false at both rates" : "TRUE", polishedBands,
                   polishedBands >= 1000 ? "" : " (VOID: under 1000)")
                 + (first.empty() ? std::string() : "  first: " + first));
    }

    //==========================================================================
    // F2: finite / sign / count / weight sweep with the polish on
    //==========================================================================
    bool checkDesignPolished (const eq::BandDesign& d, int expectedCount)
    {
        if (! checkDesign (d, expectedCount))
            return false;

        for (int i = 0; i < d.numSections; ++i)
        {
            const eq::SectionParams& p = d.params[ix (i)];
            const double weight = static_cast<double> (d.polishWeight[ix (i)]);

            if (! (std::isfinite (weight) && weight >= 0.0 && weight <= 1.0))
                return false;

            if (d.cls[ix (i)] == eq::SectionClass::gain2)
            {
                if (! (p.hasSvf && p.hasOnePole && p.mL > 0.0 && p.mB > 0.0 && p.mH > 0.0 && p.cL > 0.0 && p.cH > 0.0))
                    return false;
            }
            else if (weight > 0.0)
            {
                return false;       // only gain2 sections are polished
            }
        }

        return true;
    }

    void rowF2()
    {
        constexpr int numRandom = 1000000;
        constexpr int chunkSize = 5000;

        std::vector<eq::BandSettings> settings;
        settings.reserve (ix (numRandom + 1024));
        Rng rng (0x0F2F2F2F2ull);
        long long perType[8] = {};

        for (int n = 0; n < numRandom; ++n)
        {
            const eq::BandSettings s = randomSettings (rng);
            ++perType[static_cast<int> (s.type)];
            settings.push_back (s);
        }

        // Every combination of the range ends.
        for (int type = 0; type < 8; ++type)
            for (int si = 0; si < 8; ++si)
                for (const double fc : { 10.0, 30000.0 })
                    for (const double q : { 0.025, 40.0 })
                        for (const double gain : { -60.0, -0.0, 0.0, 60.0 })
                            settings.push_back (mk (static_cast<eq::Type> (type), fc, gain, q, si));

        struct ChunkResult
        {
            long long designs = 0, bad = 0;
            int firstSetting = -1;
            double firstRate = 0.0;
        };

        const int total = static_cast<int> (settings.size());
        const int numChunks = (total + chunkSize - 1) / chunkSize;
        std::vector<ChunkResult> results (ix (numChunks));

        parallelFor (numChunks, [&settings, &results, total] (int c)
        {
            std::array<eq::Designer, 8> designers;      // this worker's own

            for (std::size_t r = 0; r < kRates8.size(); ++r)
                designers[r].prepare (kRates8[r]);

            eq::BandDesign d {};
            ChunkResult& out = results[ix (c)];
            const int end = std::min (total, (c + 1) * chunkSize);

            for (int i = c * chunkSize; i < end; ++i)
            {
                const eq::BandSettings& s = settings[ix (i)];
                const int expected = expectedSections (s.type, s.slopeIndex);

                for (std::size_t r = 0; r < kRates8.size(); ++r)
                {
                    designers[r].design (s, d);
                    ++out.designs;

                    if (! checkDesignPolished (d, expected))
                    {
                        ++out.bad;

                        if (out.firstSetting < 0)
                        {
                            out.firstSetting = i;
                            out.firstRate = kRates8[r];
                        }
                    }
                }
            }
        });

        long long designs = 0, bad = 0;
        std::string first;

        for (const ChunkResult& r : results)
        {
            designs += r.designs;
            bad += r.bad;

            if (first.empty() && r.firstSetting >= 0)
                first = describeSettings (settings[ix (r.firstSetting)]) + strf (" fs %.0f", r.firstRate);
        }

        long long fewest = perType[0];

        for (const long long count : perType)
            fewest = std::min (fewest, count);

        row (bad == 0 && fewest >= 1000 && designs == static_cast<long long> (total) * 8,
             "F2 polished, 1e6 random settings x 8 rates + range ends: finite, g k G > 0, gain2 m c > 0, weight in [0, 1], count",
             strf ("%lld designs, %lld bad, fewest draws per type %lld%s", designs, bad, fewest,
                   fewest >= 1000 ? "" : " (VOID: under 1000)")
                 + (first.empty() ? std::string() : "  first: " + first));
    }

    //==========================================================================
    // S1: skip exactness
    //==========================================================================
    void rowS1()
    {
        constexpr int wanted = 10000;
        constexpr long long maxDraws = 2000000;

        std::array<eq::Designer, 5> designers;

        for (std::size_t r = 0; r < kRates5.size(); ++r)
            designers[r].prepare (kRates5[r]);

        Rng rng (0x51515151ull);
        eq::BandDesign polished {}, seedDesign {};
        long long drawn = 0;
        int found = 0, different = 0;
        std::string first;

        while (found < wanted && drawn < maxDraws)
        {
            const eq::BandSettings s = randomSettings (rng);
            const std::size_t r = static_cast<std::size_t> (drawn % 5);
            ++drawn;

            if (! eq::isGainType (s.type) || ! (std::fabs (s.gainDbEff) > 0.0))
                continue;

            designers[r].design (s, polished);

            int gain2Sections = 0;
            bool allZero = true;

            for (int i = 0; i < polished.numSections; ++i)
                if (polished.cls[ix (i)] == eq::SectionClass::gain2)
                {
                    ++gain2Sections;

                    if (! (polished.polishWeight[ix (i)] <= 0.0f))
                        allZero = false;
                }

            if (gain2Sections == 0 || ! allZero)
                continue;

            designers[r].designSeed (s, seedDesign);
            ++found;

            if (! sameDesign (polished, seedDesign, true))
            {
                ++different;

                if (first.empty())
                    first = describeSettings (s) + strf (" fs %.0f", kRates5[r]);
            }
        }

        row (different == 0 && found >= 1000,
             "S1 skip exactness: every gain2 weight 0 -> design == designSeed, used parameters bit for bit",
             strf ("%d found / %lld drawn, %d differ%s", found, drawn, different, found >= 1000 ? "" : " (VOID: under 1000 found)")
                 + (first.empty() ? std::string() : "  first: " + first));
    }

    //==========================================================================
    // TL2: tilt error == high-shelf error
    //==========================================================================
    void rowTL2()
    {
        const std::vector<double> fcs = geomspace (20.0, 20000.0, 40);
        const std::vector<double> qs = g1Qs (G1Kind::shelf12Wide);
        double worst = 0.0;
        long long bands = 0;
        std::string worstWhere;
        Tied tied;

        for (const double fs : { 44100.0, 48000.0 })
        {
            const Grid grid = makeGrid (fs, 1000, 20.0, 20000.0, 0.499);
            eq::Designer designer;
            designer.prepare (fs);
            eq::BandDesign tilt {}, shelf {};

            for (const int si : { 1, 5 })
                for (const double gainAbs : kG1Gains)
                    for (const double sign : { 1.0, -1.0 })
                        for (const double fc : fcs)
                            for (const double q : qs)
                            {
                                designer.design (mk (eq::Type::tilt, fc, sign * gainAbs, q, si), tilt);
                                designer.design (mk (eq::Type::highShelf, fc, sign * gainAbs, q, si), shelf);
                                ++bands;
                                double bandWorst = 0.0;

                                // err_tilt - err_hs = 10 log10 ((Dt / At) / (Dh / Ah)), one logarithm.
                                const auto point = [&] (double w, double t2)
                                {
                                    const double numerator = bandDigitalMag2 (tilt, t2) * bandAnalogMag2 (shelf, w);
                                    const double denominator = bandAnalogMag2 (tilt, w) * bandDigitalMag2 (shelf, t2);
                                    const double e = std::fabs (10.0 * std::log10 (numerator / denominator));

                                    if (! (e <= bandWorst))
                                        bandWorst = std::isfinite (e) ? e : kInf;
                                };

                                for (std::size_t i = 0; i < grid.w.size(); ++i)
                                    point (grid.w[i], grid.t2[i]);

                                collectTied (shelf, grid.wLo, grid.wHi, tied);

                                for (int i = 0; i < tied.n; ++i)
                                    point (tied.w[ix (i)], tied.t2[ix (i)]);

                                if (bandWorst > worst)
                                {
                                    worst = bandWorst;
                                    worstWhere = strf ("fs %.0f slope-index %d fc %.9g q %.9g gain %.9g", fs, si, fc, q, sign * gainAbs);
                                }
                            }
        }

        row (worst <= 1.0e-9 && bands > 0,
             "TL2 tilt error == high-shelf error within 1e-9 dB (12 and 48 dB/oct, G1 shelf grid, 44.1 / 48 kHz)",
             strf ("worst %.3g dB over %lld bands%s%s", worst, bands, worstWhere.empty() ? "" : " at ", worstWhere.c_str()));
    }

    //==========================================================================
    // Phase 2.1b INFO rows
    //==========================================================================
    struct Shares
    {
        long long polished = 0, blended = 0, seed = 0;
    };

    void countShares (const eq::BandDesign& d, Shares& shares)
    {
        for (int i = 0; i < d.numSections; ++i)
        {
            if (d.cls[ix (i)] != eq::SectionClass::gain2)
                continue;

            const double weight = static_cast<double> (d.polishWeight[ix (i)]);

            if (weight >= 0.999)
                ++shares.polished;
            else if (weight > 0.0)
                ++shares.blended;
            else
                ++shares.seed;
        }
    }

    std::string shareText (const Shares& shares)
    {
        const double total = static_cast<double> (shares.polished + shares.blended + shares.seed);

        if (! (total > 0.0))
            return "-/-/-";

        return strf ("%.0f/%.0f/%.0f", 100.0 * static_cast<double> (shares.polished) / total,
                     100.0 * static_cast<double> (shares.blended) / total, 100.0 * static_cast<double> (shares.seed) / total);
    }

    void infoH1()
    {
        info ("H1 share of gain2 sections, polished (w >= 0.999) / blended / seed (w = 0) in %, at +6 | +12 | +30 dB");

        struct Kind { G1Kind kind; const char* label; };
        static const Kind kinds[2] = { { G1Kind::bell, "bells         " }, { G1Kind::shelf12Wide, "shelves 12 dB " } };
        static const double gains[3] = { 6.0, 12.0, 30.0 };
        const std::vector<double> fcs = geomspace (20.0, 20000.0, 40);

        for (const Kind& entry : kinds)
        {
            const std::vector<double> qs = g1Qs (entry.kind);

            for (const double fs : kRates5)
            {
                eq::Designer designer;
                designer.prepare (fs);
                eq::BandDesign d {};
                Shares shares[2][3];

                for (int gi = 0; gi < 3; ++gi)
                    for (const double fc : fcs)
                        for (const double q : qs)
                        {
                            Shares& cell = shares[fc <= 10000.0 ? 0 : 1][gi];

                            if (entry.kind == G1Kind::bell)
                            {
                                designer.design (mk (eq::Type::bell, fc, gains[gi], q, 1), d);
                                countShares (d, cell);
                            }
                            else
                            {
                                for (const eq::Type type : { eq::Type::lowShelf, eq::Type::highShelf })
                                {
                                    designer.design (mk (type, fc, gains[gi], q, 1), d);
                                    countShares (d, cell);
                                }
                            }
                        }

                for (int zone = 0; zone < 2; ++zone)
                    note (strf ("%s %5.1f k  %s  %s | %s | %s", entry.label, fs / 1000.0, zone == 0 ? "fc <= 10 k" : "10-20 k   ",
                                shareText (shares[zone][0]).c_str(), shareText (shares[zone][1]).c_str(),
                                shareText (shares[zone][2]).c_str()));

                if (entry.kind == G1Kind::bell && fs < 44200.0)
                {
                    std::string whole;

                    for (int gi = 0; gi < 3; ++gi)
                    {
                        Shares sum = shares[0][gi];
                        sum.polished += shares[1][gi].polished;
                        sum.blended += shares[1][gi].blended;
                        sum.seed += shares[1][gi].seed;
                        whole += (gi == 0 ? "" : " | ") + shareText (sum);
                    }

                    note ("bells           44.1 k  whole grid  " + whole
                              + "   (research Q4: polished 25 -> 44, blended 7, seed 68 -> 49 from +-6 to +-30 dB)");
                }
            }
        }
    }

    struct SeedPolished
    {
        double seed = 0.0, polished = 0.0;
        long long settings = 0, worse = 0;
    };

    /** Worst band error of the seed and of the polished design over a box of
        settings (grid + tied points), and how many settings read more than
        1e-3 dB worse polished than seed. */
    SeedPolished seedVsPolished (double fs, const std::vector<eq::Type>& types, int slopeIndex,
                                 const std::vector<double>& fcs, const std::vector<double>& qs,
                                 const std::vector<double>& gains, const Grid& grid)
    {
        eq::Designer designer;
        designer.prepare (fs);
        eq::BandDesign seedDesign {}, polished {};
        SeedPolished out;

        for (const eq::Type type : types)
            for (const double gain : gains)
                for (const double fc : fcs)
                    for (const double q : qs)
                    {
                        const eq::BandSettings s = mk (type, fc, gain, q, slopeIndex);
                        designer.designSeed (s, seedDesign);
                        designer.design (s, polished);

                        const double eSeed = maxError (seedDesign, seedDesign, grid, true);
                        const double ePolished = maxError (polished, polished, grid, true);
                        out.seed = std::max (out.seed, eSeed);
                        out.polished = std::max (out.polished, ePolished);
                        ++out.settings;

                        if (ePolished > eSeed + 1.0e-3)
                            ++out.worse;
                    }

        return out;
    }

    /** "seed/polished" at +-6, +-12, +-30 dB. When given, polishedOut receives
        the three polished figures. */
    std::string seedPolishedTriple (double fs, const std::vector<eq::Type>& types, int slopeIndex,
                                    const std::vector<double>& fcs, const std::vector<double>& qs, const Grid& grid,
                                    double* polishedOut = nullptr)
    {
        static const double gains[3] = { 6.0, 12.0, 30.0 };
        std::string text;

        for (int gi = 0; gi < 3; ++gi)
        {
            const SeedPolished r = seedVsPolished (fs, types, slopeIndex, fcs, qs, { gains[gi], -gains[gi] }, grid);
            text += strf ("%s%.2f/%.2f", text.empty() ? "" : ", ", r.seed, r.polished);

            if (polishedOut != nullptr)
                polishedOut[gi] = r.polished;
        }

        return text;
    }

    void infoOdd()
    {
        const double fs = 44100.0;
        const Grid grid = makeGrid (fs, 1000, 20.0, 20000.0, 0.499);
        const std::vector<double> q1 { 1.0 };
        const std::vector<eq::Type> shelves { eq::Type::lowShelf, eq::Type::highShelf };
        static const double gains[3] = { 6.0, 12.0, 30.0 };
        static const double ceilings[3] = { 0.10, 0.15, 0.30 };

        info ("I-odd 18 dB/oct shelves, Q 1, 44.1 kHz, polished band error at +-6 / +-12 / +-30 dB; ceiling 0.10 / 0.15 / 0.30");

        for (int zone = 0; zone < 2; ++zone)
        {
            const std::vector<double> fcs = zone == 0 ? geomspace (20.0, 10000.0, 20) : geomspace (10000.0 * 1.0001, 20000.0, 20);
            std::string text = zone == 0 ? "fc <= 10 k:" : "10-20 k:   ";
            bool above = false;

            for (int gi = 0; gi < 3; ++gi)
            {
                const double worst = zoneWorst (fs, shelves, 2, fcs, q1, { gains[gi], -gains[gi] }, grid, true, -1.0e9, true);
                text += strf ("  %.3f", worst);

                if (! (worst <= ceilings[gi]))
                    above = true;
            }

            text += zone == 0 ? "   (research Q8, seed/polished: 0.07/0.05, 0.14/0.10, 0.43/0.24)"
                              : "   (research Q8, seed/polished: 0.48/0.06, 0.92/0.15, 1.64/0.28)";

            if (above)
                text += "  WARN: above the ceiling";

            note (text);
        }
    }

    void infoWeak()
    {
        const std::vector<eq::Type> shelves { eq::Type::lowShelf, eq::Type::highShelf };
        const std::vector<double> lowFcs = geomspace (20.0, 10000.0, 20);
        const std::vector<double> highFcs = geomspace (10000.0 * 1.0001, 20000.0, 20);

        const std::vector<double> aboveFcs = geomspace (20000.0 * 1.0001, 30000.0, 20);

        struct Weak { const char* label; int slopeIndex; std::vector<double> qs; const char* bench; };
        const Weak weak[3] =
        {
            { "12 dB/oct, Q 2-10        ", 1, geomspace (2.0, 10.0, 5),
              "bench, polished, 44.1 k: 0.04, 0.07-0.10, 0.25-0.39 | 0.18, 0.44, 0.45" },
            { "12 dB/oct, Q 20-40       ", 1, geomspace (20.0, 40.0, 3),
              "bench, polished, 44.1 k: 0.04, 0.07-0.10, 0.25-0.39 | 0.75, 1.03, 0.43" },
            { "48 dB/oct, Q .25 .5 2 4  ", 5, { 0.25, 0.5, 2.0, 4.0 }, nullptr }
        };

        // WARN ceilings, polished, 44.1 kHz, fc 10-20 k (bench: at most 0.75 / 1.03 / 0.45).
        static const double ceilings[3] = { 1.0, 1.5, 1.0 };

        info ("I-weak resonant shelves, seed/polished band error at +-6, +-12, +-30 dB: fc <= 10 k | 10-20 k; "
              "polished ceiling at 44.1 kHz, 10-20 k: 1.0 / 1.5 / 1.0 dB");

        double stubborn = 0.0;

        for (const double fs : { 44100.0, 48000.0 })
        {
            const Grid grid = makeGrid (fs, 1000, 20.0, 20000.0, 0.499);
            const bool gatedRate = fs < 44200.0;

            for (int wi = 0; wi < 3; ++wi)
            {
                const Weak& w = weak[wi];
                double polishedHigh[3] = { 0.0, 0.0, 0.0 };

                note (strf ("%4.1f k  %s ", fs / 1000.0, w.label)
                          + seedPolishedTriple (fs, shelves, w.slopeIndex, lowFcs, w.qs, grid) + " | "
                          + seedPolishedTriple (fs, shelves, w.slopeIndex, highFcs, w.qs, grid, polishedHigh));

                if (! gatedRate)
                    continue;

                if (w.bench != nullptr)
                    note (std::string ("        ") + w.bench);

                if (wi == 0)
                    stubborn = polishedHigh[2];

                for (int gi = 0; gi < 3; ++gi)
                    if (! (polishedHigh[gi] <= ceilings[gi]))
                        note (strf ("        WARN: %s 10-20 k, column %d: %.2f dB polished is above the ceiling %.1f dB",
                                    w.label, gi + 1, polishedHigh[gi], ceilings[gi]));
            }

            note (strf ("%4.1f k  12 dB/oct, Q 2-40, fc 20-30 k: ", fs / 1000.0)
                      + seedPolishedTriple (fs, shelves, 1, aboveFcs, { 2.0, 4.0, 10.0, 20.0, 40.0 }, grid)
                      + (gatedRate ? "   (bench, polished: 0.58, 0.22, 0.38)" : ""));
        }

        note (strf ("the stubborn cell (PLAN-2.1b P4, reported, not gated): 12 dB/oct, Q 2-10, +-30 dB, 10-20 k at 44.1 kHz: "
                    "%.2f dB polished against the bench's 0.45 dB", stubborn));
        note ("bench at 48 kHz: weak rows at or below 0.34 / 0.52 / 0.17 dB polished");
    }

    void infoNew()
    {
        const std::vector<double> fcs = geomspace (20.0, 20000.0, 40);
        const std::vector<double> q1 { 1.0 };
        const std::vector<eq::Type> shelves { eq::Type::lowShelf, eq::Type::highShelf };
        const std::vector<eq::Type> bell { eq::Type::bell };
        static const double gains[3] = { 6.0, 12.0, 30.0 };
        static const int dbPerOct[8] = { 6, 12, 18, 24, 36, 48, 72, 96 };
        const std::vector<double> allGains { 3.0, -3.0, 6.0, -6.0, 12.0, -12.0, 18.0, -18.0, 30.0, -30.0 };

        info ("I-new cases research did not run (new evidence; not gated)");

        note ("Tilt as a type, Q 1, fc 20 Hz-20 kHz x 40, polished band error at +-6 / +-12 / +-30 dB:");

        for (const double fs : { 44100.0, 48000.0 })
        {
            const Grid grid = makeGrid (fs, 1000, 20.0, 20000.0, 0.499);

            for (int si = 0; si < 8; ++si)
            {
                std::string text = strf ("  %4.1f k  %2d dB/oct:", fs / 1000.0, dbPerOct[si]);

                for (const double gainAbs : gains)
                    text += strf ("  %.3f", zoneWorst (fs, { eq::Type::tilt }, si, fcs, q1, { gainAbs, -gainAbs }, grid, true, -1.0e9, true));

                note (text);
            }
        }

        note ("96 dB/oct shelves, Q 4 / 10 / 40, fc 20 Hz-20 kHz x 40, seed/polished at +-6, +-12, +-30 dB:");

        for (const double fs : { 44100.0, 48000.0 })
        {
            const Grid grid = makeGrid (fs, 1000, 20.0, 20000.0, 0.499);
            note (strf ("  %4.1f k  ", fs / 1000.0) + seedPolishedTriple (fs, shelves, 7, fcs, { 4.0, 10.0, 40.0 }, grid));
        }

        note ("bells with fc 20-30 kHz (re-centred seed), fc x 20, Q 0.025-40 x 17, gains +-3 ... +-30 dB, seed -> polished:");

        for (const double fs : { 44100.0, 48000.0 })
        {
            const Grid grid = makeGrid (fs, 1000, 20.0, 20000.0, 0.499);
            const SeedPolished r = seedVsPolished (fs, bell, 1, geomspace (20000.0 * 1.0001, 30000.0, 20),
                                                   g1Qs (G1Kind::bell), allGains, grid);
            note (strf ("  %4.1f k  %.2f -> %.2f dB, %lld of %lld settings worse than the seed by more than 1e-3 dB   (model, random sweep: %s)",
                        fs / 1000.0, r.seed, r.polished, r.worse, r.settings, fs < 44200.0 ? "2.66 -> 0.40" : "2.51 -> 0.17"));
        }

        note ("22.05 and 32 kHz: nothing is polished below 44.1 kHz; design == designSeed bit for bit, every weight 0 (row PR1)");
    }

    void infoCost()
    {
        const double fs = 44100.0;
        constexpr int numBells = 400000;
        constexpr int numShelves = 100000;

        // The draw of the Phase 2.1 seed timing row.
        std::vector<eq::BandSettings> bells, shelves, skipping;
        bells.reserve (ix (numBells));
        shelves.reserve (ix (numShelves));
        Rng rng (0x5EEDull);

        for (int i = 0; i < numBells; ++i)
        {
            const double fc = 20.0 * std::pow (1000.0, rng.uniform());
            const double q = 0.025 * std::pow (1600.0, rng.uniform());
            const double gain = 1.0 + 29.0 * rng.uniform();
            bells.push_back (mk (eq::Type::bell, fc, gain, q, 1));
        }

        for (int i = 0; i < numShelves; ++i)
        {
            const double fc = 20.0 * std::pow (1000.0, rng.uniform());
            const double q = 0.025 * std::pow (1600.0, rng.uniform());
            const double gain = 1.0 + 29.0 * rng.uniform();
            shelves.push_back (mk (i % 2 == 0 ? eq::Type::lowShelf : eq::Type::highShelf, fc, gain, q, 7));
        }

        eq::Designer designer;
        designer.prepare (fs);
        eq::BandDesign d {};
        double sink = 0.0;

        // Untimed: the shares, and the bells whose section keeps the seed.
        const auto shareOf = [&designer, &d] (const std::vector<eq::BandSettings>& set, std::vector<eq::BandSettings>* keepers)
        {
            long long sections = 0, weighted = 0;

            for (const eq::BandSettings& s : set)
            {
                designer.design (s, d);
                bool anyWeight = false;

                for (int i = 0; i < d.numSections; ++i)
                    if (d.cls[ix (i)] == eq::SectionClass::gain2)
                    {
                        ++sections;

                        if (d.polishWeight[ix (i)] > 0.0f)
                        {
                            ++weighted;
                            anyWeight = true;
                        }
                    }

                if (keepers != nullptr && ! anyWeight)
                    keepers->push_back (s);
            }

            return sections > 0 ? 100.0 * static_cast<double> (weighted) / static_cast<double> (sections) : 0.0;
        };

        const double bellShare = shareOf (bells, &skipping);
        const double shelfShare = shareOf (shelves, nullptr);

        const auto timeSet = [&designer, &d, &sink] (const std::vector<eq::BandSettings>& set, bool polished)
        {
            long long sections = 0;
            const auto start = std::chrono::steady_clock::now();

            for (const eq::BandSettings& s : set)
            {
                designBand (designer, polished, s, d);
                sink += d.params[0].mB;
                sections += d.numSections;
            }

            const auto stop = std::chrono::steady_clock::now();
            return std::chrono::duration<double, std::nano> (stop - start).count()
                     / static_cast<double> (std::max (sections, 1LL));
        };

        const double bellSeedNs = timeSet (bells, false);
        const double bellPolishedNs = timeSet (bells, true);
        const double skipNs = timeSet (skipping, true);
        const double shelfSeedNs = timeSet (shelves, false);
        const double shelfPolishedNs = timeSet (shelves, true);

        info ("I-cost ns per section, whole Designer call (prototype + design), one thread, 44.1 kHz",
              strf ("[checksum %.6g]", sink));
        note (strf ("(i)   seed, 400 000 random bells:                 %8.1f ns   (research port, section only: 30-36 ns)", bellSeedNs));
        note (strf ("(ii)  polished, the same bells:                   %8.1f ns   %.1f %% of sections with weight > 0   (model 0.8 us per polished section)%s",
                    bellPolishedNs, bellShare, bellPolishedNs > 2000.0 ? "  WARN: above 2 us" : ""));
        note (strf ("(iii) polished, the %zu bells whose weight is 0: %8.1f ns   (seed kept: skipped, or fitted and not accepted)",
                    skipping.size(), skipNs));
        note (strf ("(iv)  100 000 random 96 dB/oct shelves (8 sections): seed %.1f ns, polished %.1f ns per section; "
                    "%.1f %% of sections with weight > 0%s",
                    shelfSeedNs, shelfPolishedNs, shelfShare, shelfPolishedNs > 2000.0 ? "  WARN: above 2 us" : ""));
    }

    //==========================================================================
    // Phase 2.2 (PLAN-2.2 T4): the engine in double
    //
    // Every row drives eq::Band + eq::Router from Designer::design. A setting
    // goes through eq::makeBandInput, the one raw -> settings function. No
    // audio golden: a bit-identity row compares two renders of this binary.
    //==========================================================================
    constexpr int kEnMaxBands = 24;
    constexpr int kEnNumRates = 5;
    constexpr int kEnNumRates8 = 8;
    constexpr float kEnScale100 = 100.0f;

    static_assert (kRates5.size() == static_cast<std::size_t> (kEnNumRates), "the engine rows index kRates5");
    static_assert (kRates8.size() == static_cast<std::size_t> (kEnNumRates8), "the engine rows index kRates8");

    const char* const kEnPlacementNames[5] = { "stereo", "left", "right", "mid", "side" };

    /** An enabled, not bypassed band as the parameter atomics would hold it. */
    eq::RawBand enRaw (eq::Type type, double freqHz, double gainDb, double q, int slopeIndex, eq::Placement placement)
    {
        eq::RawBand raw;
        raw.enabled = 1.0f;
        raw.bypass = 0.0f;
        raw.type = static_cast<float> (static_cast<int> (type));
        raw.freq = static_cast<float> (freqHz);
        raw.gain = static_cast<float> (gainDb);
        raw.q = static_cast<float> (q);
        raw.slope = static_cast<float> (slopeIndex);
        raw.placement = static_cast<float> (static_cast<int> (placement));
        return raw;
    }

    /** Full range: type, slope and placement uniform; fc 10 Hz - 30 kHz log;
        Q 0.025 - 40 log; gain +-30 dB. Enabled, not bypassed. */
    eq::RawBand enRandomRaw (Rng& rng)
    {
        eq::RawBand raw;
        raw.enabled = 1.0f;
        raw.bypass = 0.0f;
        raw.type = static_cast<float> (rng.below (8));
        raw.slope = static_cast<float> (rng.below (8));
        raw.placement = static_cast<float> (rng.below (5));
        raw.freq = static_cast<float> (10.0 * std::pow (3000.0, rng.uniform()));
        raw.q = static_cast<float> (0.025 * std::pow (1600.0, rng.uniform()));
        raw.gain = static_cast<float> (-30.0 + 60.0 * rng.uniform());
        return raw;
    }

    /** Independent of eq::sameBandInput on purpose: bit patterns, field by field. */
    bool enSameFields (const eq::BandInput& a, const eq::BandInput& b)
    {
        return a.active == b.active
            && a.placement == b.placement
            && a.settings.type == b.settings.type
            && a.settings.slopeIndex == b.settings.slopeIndex
            && sameBits (a.settings.freqHz, b.settings.freqHz)
            && sameBits (a.settings.gainDbEff, b.settings.gainDbEff)
            && sameBits (a.settings.q, b.settings.q);
    }

    /** White noise with a -12 dBFS peak (uniform, +-0.25), float-valued as a
        host buffer is. */
    void enFillNoise (std::vector<double>& out, Rng& rng)
    {
        for (double& v : out)
            v = static_cast<double> (static_cast<float> ((rng.uniform() * 2.0 - 1.0) * 0.25));
    }

    /** Up to 24 bands in index order, each at rest when added. */
    struct EnChain
    {
        int count = 0;
        std::array<eq::Band, kEnMaxBands> bands;
        std::array<eq::BandInput, kEnMaxBands> inputs {};

        void add (const eq::Designer& designer, const eq::BandInput& in)
        {
            if (count >= kEnMaxBands)
                return;

            eq::BandDesign d {};
            designer.design (in.settings, d);
            bands[ix (count)].setDesign (d);
            bands[ix (count)].reset();
            inputs[ix (count)] = in;
            ++count;
        }

        void process (double* lane0, double* lane1, int numSamples, bool stereo)
        {
            for (int b = 0; b < count; ++b)
                if (inputs[ix (b)].active)
                    eq::Router::process (bands[ix (b)], inputs[ix (b)].placement, lane0, lane1, numSamples, stereo);
        }
    };

    //==========================================================================
    // EN-ir / EN-irn: rendered impulse response against the closed form
    //==========================================================================
    constexpr int kEnNumIrCases = 8;

    struct EnIrCase { const char* name; eq::Type type; double freqHz, gainDb, q; int slopeIndex; };

    /** PLAN-2.2 P11: the eight binding cases. */
    const EnIrCase kEnIrCaseList[kEnNumIrCases] =
    {
        { "Bell 1 kHz +12 dB Q 1",              eq::Type::bell,      1000.0,  12.0,  1.0, 1 },
        { "Low Shelf 200 Hz +9 dB 24 dB/oct",   eq::Type::lowShelf,   200.0,   9.0,  1.0, 3 },
        { "High Shelf 8 kHz -12 dB 12 dB/oct",  eq::Type::highShelf, 8000.0, -12.0,  1.0, 1 },
        { "Low Cut 80 Hz 48 dB/oct",            eq::Type::lowCut,      80.0,   0.0,  1.0, 5 },
        { "High Cut 6 kHz 96 dB/oct",           eq::Type::highCut,   6000.0,   0.0,  1.0, 7 },
        { "Notch 1 kHz Q 10",                   eq::Type::notch,     1000.0,   0.0, 10.0, 1 },
        { "Band Pass 500 Hz Q 2",               eq::Type::bandPass,   500.0,   0.0,  2.0, 1 },
        { "Tilt 1 kHz +6 dB 12 dB/oct",         eq::Type::tilt,      1000.0,   6.0,  1.0, 1 }
    };

    eq::BandInput enIrInput (const EnIrCase& c, double freqFactor)
    {
        return eq::makeBandInput (enRaw (c.type, c.freqHz * freqFactor, c.gainDb, c.q, c.slopeIndex, eq::Placement::stereo),
                                  kEnScale100, 0.0);
    }

    struct EnIrResult
    {
        double worstDb = 0.0;       // largest |rendered - closed form| where the closed form is above -80 dB
        double seconds = 0.0;       // rendered length
        double tailDb = 0.0;        // the last 10 ms, re the peak
        int scored = 0;             // DFT points above -80 dB
        bool tailReached = false;
        bool finite = true;
    };

    /** A unit impulse through one band (Router, mono layout). The render is
        extended in 1 s steps until its last 10 ms is at least 200 dB below
        the peak; at 20 s it stops and tailReached stays false. Then a
        single-bin DFT at 160 log-spaced points 20 Hz -> min (20 kHz,
        0.499 fs), scored against eq::bandMagDb of `reference` where that is
        above -80 dB. The phasor is re-seeded from cos / sin every 1024
        samples, so its drift does not reach the result. */
    EnIrResult enIrMeasure (double fs, const eq::BandDesign& rendered, const eq::BandDesign& reference)
    {
        constexpr int numPoints = 160;
        constexpr int maxSeconds = 20;
        constexpr double tailRatio = 1.0e-10;       // 200 dB

        EnIrResult result;
        eq::Band band;
        band.setDesign (rendered);
        band.reset();

        const int step = static_cast<int> (std::lround (fs));
        const int tailLength = std::max (1, static_cast<int> (std::lround (0.01 * fs)));
        std::vector<double> h;
        h.reserve (ix (step) * 2);
        double peak = 0.0;

        for (int second = 0; second < maxSeconds && result.finite && ! result.tailReached; ++second)
        {
            const std::size_t start = h.size();
            h.resize (start + ix (step), 0.0);

            if (second == 0)
                h[0] = 1.0;

            eq::Router::process (band, eq::Placement::stereo, h.data() + start, nullptr, step, false);

            for (std::size_t i = start; i < h.size(); ++i)
            {
                const double a = std::fabs (h[i]);

                if (! std::isfinite (a))
                    result.finite = false;
                else if (a > peak)
                    peak = a;
            }

            double tail = 0.0;

            for (std::size_t i = h.size() - ix (tailLength); i < h.size(); ++i)
                tail = std::max (tail, std::fabs (h[i]));

            result.seconds = static_cast<double> (second + 1);
            result.tailDb = 20.0 * std::log10 (std::max (tail, kTiny) / std::max (peak, kTiny));
            result.tailReached = result.finite && peak > 0.0 && tail <= peak * tailRatio;
        }

        if (! result.finite)
        {
            result.worstDb = kInf;
            return result;
        }

        const double fHi = std::min (20000.0, 0.499 * fs);
        const std::size_t total = h.size();

        for (int p = 0; p < numPoints; ++p)
        {
            const double hz = 20.0 * std::pow (fHi / 20.0, static_cast<double> (p) / static_cast<double> (numPoints - 1));
            const double w = 2.0 * kPi * hz / fs;
            const double ref = eq::bandMagDb (reference, w);

            if (! (ref > -80.0))
                continue;

            const double cr = std::cos (w), ci = -std::sin (w);
            double pr = 1.0, pim = 0.0, ar = 0.0, ai = 0.0;

            for (std::size_t n = 0; n < total; ++n)
            {
                if ((n & 1023u) == 0u && n != 0u)
                {
                    const double angle = w * static_cast<double> (n);
                    pr = std::cos (angle);
                    pim = -std::sin (angle);
                }

                const double v = h[n];
                ar += v * pr;
                ai += v * pim;
                const double t = pr * cr - pim * ci;
                pim = pr * ci + pim * cr;
                pr = t;
            }

            const double e = std::fabs (eq::powerToDb (ar * ar + ai * ai) - ref);
            ++result.scored;

            if (! (e <= result.worstDb))
                result.worstDb = std::isfinite (e) ? e : kInf;
        }

        return result;
    }

    void rowsEnIr()
    {
        constexpr int numMain = kEnNumRates * kEnNumIrCases;
        constexpr double limitDb = 1.0e-6;

        std::vector<EnIrResult> results (ix (numMain));
        std::vector<EnIrResult> control (ix (kEnNumRates));

        // Tasks 0 ... 39: rate-major, the eight cases. Tasks 40 ... 44: the control, one per rate.
        parallelFor (numMain + kEnNumRates, [&results, &control] (int task)
        {
            const bool isControl = task >= numMain;
            const int rateIndex = isControl ? task - numMain : task / kEnNumIrCases;
            const EnIrCase& c = kEnIrCaseList[isControl ? 0 : task % kEnNumIrCases];
            const double fs = kRates5[ix (rateIndex)];

            eq::Designer designer;
            designer.prepare (fs);
            eq::BandDesign rendered {};
            designer.design (enIrInput (c, 1.0).settings, rendered);

            if (! isControl)
            {
                results[ix (task)] = enIrMeasure (fs, rendered, rendered);
                return;
            }

            eq::BandDesign higher {};
            designer.design (enIrInput (c, 1.01).settings, higher);
            control[ix (rateIndex)] = enIrMeasure (fs, rendered, higher);
        });

        bool ok = true;
        double worst = 0.0, longest = 0.0, leastTail = -1.0e9;
        int scored = 0, tailMissed = 0, nonFinite = 0;
        std::string worstWhere;

        for (int task = 0; task < numMain; ++task)
        {
            const EnIrResult& r = results[ix (task)];
            ok = ok && r.finite && r.tailReached && r.scored > 0 && r.worstDb <= limitDb;
            scored += r.scored;
            longest = std::max (longest, r.seconds);
            leastTail = std::max (leastTail, r.tailDb);

            if (! r.finite)
                ++nonFinite;
            else if (! r.tailReached)
                ++tailMissed;

            if (! (r.worstDb <= worst))
            {
                worst = r.worstDb;
                worstWhere = strf ("%s at %.0f Hz", kEnIrCaseList[task % kEnNumIrCases].name, kRates5[ix (task / kEnNumIrCases)]);
            }
        }

        row (ok, "EN-ir rendered impulse response vs eq::bandMagDb: 8 types x 5 rates, <= 1e-6 dB above -80 dB; tail >= 200 dB down",
             strf ("worst %.3g dB%s%s; %d points scored; %d of %d renders non-finite, %d stopped at the 20 s cap; longest render %.0f s, "
                   "least tail %.0f dB re peak (1 s steps, last 10 ms)",
                   worst, worstWhere.empty() ? "" : " at ", worstWhere.c_str(), scored, nonFinite, numMain, tailMissed, longest,
                   std::max (leastTail, -999.0)));

        for (int c = 0; c < kEnNumIrCases; ++c)
        {
            std::string text = strf ("%-34s", kEnIrCaseList[c].name);
            double caseSeconds = 0.0, caseTail = -1.0e9;
            std::string flags;

            for (int r = 0; r < kEnNumRates; ++r)
            {
                const EnIrResult& res = results[ix (r * kEnNumIrCases + c)];
                text += strf ("%s%.1f k %.2e", r == 0 ? " " : " | ", kRates5[ix (r)] / 1000.0, res.worstDb);
                caseSeconds = std::max (caseSeconds, res.seconds);
                caseTail = std::max (caseTail, res.tailDb);

                if (! res.finite)
                    flags += strf ("  NON-FINITE at %.0f Hz", kRates5[ix (r)]);
                else if (! res.tailReached)
                    flags += strf ("  TAIL NOT REACHED at %.0f Hz", kRates5[ix (r)]);

                if (res.scored == 0)
                    flags += strf ("  VOID at %.0f Hz: no point above -80 dB", kRates5[ix (r)]);
            }

            note (text + strf (" dB; render %.0f s, tail <= %.0f dB re peak", caseSeconds, std::max (caseTail, -999.0)) + flags);
        }

        // EN-irn: the same code, the bell against the closed form of the bell 1 % higher.
        bool fires = true;
        double smallest = kInf;
        std::string text;

        for (int r = 0; r < kEnNumRates; ++r)
        {
            const EnIrResult& res = control[ix (r)];
            const bool exceeded = res.finite && res.tailReached && res.scored > 0 && res.worstDb > limitDb;
            fires = fires && exceeded;
            smallest = std::min (smallest, res.worstDb);
            text += strf ("%s%.1f k %.3g", r == 0 ? "" : " | ", kRates5[ix (r)] / 1000.0, res.worstDb);
        }

        row (fires, "EN-irn the EN-ir scorer on the bell vs the closed form of the bell 1 % higher exceeds 1e-6 dB at every rate",
             strf ("%s; smallest error %.3g dB (limit 1e-6): %s", fires ? "the control fires" : "the control did NOT fire",
                   smallest, text.c_str()));
    }

    //==========================================================================
    // EN-ms / EN-msn: Mid / Side difference injection
    //==========================================================================
    struct EnMsConfig { const char* name; int numBands; double gainDb; };

    const EnMsConfig kEnMsConfigs[4] =
    {
        { "1 bell +30 dB",   1,  30.0 },
        { "1 bell -30 dB",   1, -30.0 },
        { "24 bells +6 dB",  24,  6.0 },
        { "24 bells +12 dB", 24, 12.0 }
    };

    struct EnMsResult
    {
        double untouchedDb = 0.0;       // largest change of the other component, re input peak
        double touchedDb = 0.0;         // largest change of the band's own component, re input peak
        bool finite = true;
    };

    /** Stereo noise at -12 dBFS, 48 kHz, bells Q 1 (40 Hz - 16 kHz log-spaced,
        a single bell at 800 Hz). `claimed` (Mid or Side) names the component
        the row is about; `actual` is the placement the bands are given. */
    EnMsResult enMsMeasure (const EnMsConfig& config, eq::Placement claimed, eq::Placement actual)
    {
        constexpr int numSamples = 1 << 17;

        eq::Designer designer;
        designer.prepare (48000.0);

        Rng rng (0x454E4D53ull);
        std::vector<double> inL (ix (numSamples)), inR (ix (numSamples));

        for (int i = 0; i < numSamples; ++i)
        {
            inL[ix (i)] = static_cast<double> (static_cast<float> ((rng.uniform() * 2.0 - 1.0) * 0.25));
            inR[ix (i)] = static_cast<double> (static_cast<float> ((rng.uniform() * 2.0 - 1.0) * 0.25));
        }

        std::vector<double> outL (inL), outR (inR);
        EnChain chain;

        for (int b = 0; b < config.numBands; ++b)
        {
            const double position = config.numBands > 1 ? static_cast<double> (b) / static_cast<double> (config.numBands - 1) : 0.5;
            const double fc = 40.0 * std::pow (400.0, position);
            chain.add (designer, eq::makeBandInput (enRaw (eq::Type::bell, fc, config.gainDb, 1.0, 1, actual), kEnScale100, 0.0));
        }

        chain.process (outL.data(), outR.data(), numSamples, true);

        const bool side = claimed == eq::Placement::side;
        double inPeak = 0.0, untouched = 0.0, touched = 0.0;
        EnMsResult result;

        for (int i = 0; i < numSamples; ++i)
        {
            const double il = inL[ix (i)], ir = inR[ix (i)];
            const double ol = outL[ix (i)], orr = outR[ix (i)];

            if (! std::isfinite (ol) || ! std::isfinite (orr))
                result.finite = false;

            inPeak = std::max (inPeak, std::max (std::fabs (il), std::fabs (ir)));

            const double midIn = 0.5 * (il + ir), sideIn = 0.5 * (il - ir);
            const double midOut = 0.5 * (ol + orr), sideOut = 0.5 * (ol - orr);
            const double untouchedError = side ? std::fabs (midOut - midIn) : std::fabs (sideOut - sideIn);
            const double touchedChange = side ? std::fabs (sideOut - sideIn) : std::fabs (midOut - midIn);
            untouched = std::max (untouched, untouchedError);
            touched = std::max (touched, touchedChange);
        }

        result.untouchedDb = 20.0 * std::log10 (std::max (untouched, kTiny) / std::max (inPeak, kTiny));
        result.touchedDb = 20.0 * std::log10 (std::max (touched, kTiny) / std::max (inPeak, kTiny));
        return result;
    }

    void rowsEnMs()
    {
        constexpr double untouchedLimitDb = -250.0;
        constexpr double touchedFloorDb = -40.0;
        static const eq::Placement kinds[2] = { eq::Placement::mid, eq::Placement::side };

        bool ok = true, fires = true;
        double worstUntouched = -1.0e9, leastTouched = 1.0e9, controlLowest = 1.0e9;
        std::vector<std::string> lines;

        for (const eq::Placement kind : kinds)
            for (const EnMsConfig& config : kEnMsConfigs)
            {
                const bool side = kind == eq::Placement::side;
                const EnMsResult r = enMsMeasure (config, kind, kind);
                const EnMsResult c = enMsMeasure (config, kind, eq::Placement::left);

                const bool cellOk = r.finite && r.untouchedDb <= untouchedLimitDb && r.touchedDb > touchedFloorDb;
                ok = ok && cellOk;
                worstUntouched = std::max (worstUntouched, r.untouchedDb);
                leastTouched = std::min (leastTouched, r.touchedDb);

                // The control fails the EN-ms verdict, and on the untouched-component test itself.
                const bool controlFails = c.finite && ! (c.untouchedDb <= untouchedLimitDb);
                fires = fires && controlFails;
                controlLowest = std::min (controlLowest, c.untouchedDb);

                lines.push_back (strf ("%-4s %-16s: %s %8.1f dB re input peak, %s changes by %+7.1f dB%s | on Left: %s %+7.1f dB%s",
                                       side ? "Side" : "Mid", config.name, side ? "mid " : "side", r.untouchedDb,
                                       side ? "side" : "mid ", r.touchedDb, cellOk ? "" : "  OFF",
                                       side ? "mid " : "side", c.untouchedDb, controlFails ? "" : "  CONTROL DID NOT FIRE"));
            }

        row (ok, "EN-ms Mid / Side bands, stereo noise -12 dBFS, 48 kHz: the other component <= -250 dB re input peak",
             strf ("worst %.1f dB (limit %.0f) over 8 settings (1 bell +-30 dB, 24 bells +6 and +12 dB; Mid and Side); "
                   "the band's own component changes by at least %+.1f dB (vacuity floor %.0f); research: -265 dB or better",
                   worstUntouched, untouchedLimitDb, leastTouched, touchedFloorDb));

        for (const std::string& text : lines)
            note (text);

        row (fires, "EN-msn EN-ms with the bands on Left FAILS every one of the 8 settings",
             strf ("%s; lowest \"untouched\" component change with the bands on Left %+.1f dB re input peak (limit %.0f)",
                   fires ? "the control fires" : "the control did NOT fire", controlLowest, untouchedLimitDb));
    }

    //==========================================================================
    // EN-lr: Left / Right bands leave the other lane alone
    //==========================================================================
    void rowEnLr()
    {
        constexpr int numSamples = 1 << 16;

        struct LrBand { eq::Type type; double freqHz, gainDb, q; int slopeIndex; };
        static const LrBand set[6] =
        {
            { eq::Type::bell,       1000.0,  9.0, 2.0, 1 },
            { eq::Type::lowShelf,    150.0, -6.0, 1.0, 3 },
            { eq::Type::highShelf,  6000.0,  6.0, 0.7, 2 },
            { eq::Type::lowCut,       60.0,  0.0, 1.0, 4 },
            { eq::Type::highCut,   14000.0,  0.0, 1.4, 1 },
            { eq::Type::tilt,        800.0,  4.0, 1.0, 1 }
        };

        eq::Designer designer;
        designer.prepare (48000.0);

        Rng rng (0x454E4C52ull);
        std::vector<double> inL (ix (numSamples)), inR (ix (numSamples));
        enFillNoise (inL, rng);
        enFillNoise (inR, rng);

        const std::size_t bytes = sizeof (double) * ix (numSamples);
        bool ok = true;
        std::string text;

        for (const eq::Placement placement : { eq::Placement::left, eq::Placement::right })
        {
            const bool left = placement == eq::Placement::left;
            std::vector<double> outL (inL), outR (inR), alone (left ? inL : inR);
            EnChain chain, chainAlone;

            for (const LrBand& b : set)
            {
                const eq::BandInput in = eq::makeBandInput (enRaw (b.type, b.freqHz, b.gainDb, b.q, b.slopeIndex, placement),
                                                            kEnScale100, 0.0);
                chain.add (designer, in);
                chainAlone.add (designer, in);
            }

            chain.process (outL.data(), outR.data(), numSamples, true);

            // The lane the placement does not touch is not even passed in.
            chainAlone.process (left ? alone.data() : nullptr, left ? nullptr : alone.data(), numSamples, true);

            const std::vector<double>& touched = left ? outL : outR;
            const std::vector<double>& touchedIn = left ? inL : inR;
            const std::vector<double>& other = left ? outR : outL;
            const std::vector<double>& otherIn = left ? inR : inL;

            const bool otherSame = std::memcmp (other.data(), otherIn.data(), bytes) == 0;
            const bool aloneSame = std::memcmp (alone.data(), touched.data(), bytes) == 0;
            int differing = 0;
            bool finite = true;

            for (int i = 0; i < numSamples; ++i)
            {
                if (! sameBits (touched[ix (i)], touchedIn[ix (i)]))
                    ++differing;

                if (! std::isfinite (touched[ix (i)]))
                    finite = false;
            }

            ok = ok && otherSame && aloneSame && finite && differing > numSamples / 2;
            text += strf ("%s%s bands: %s lane %s, %s lane differs in %d of %d samples%s, null other-lane pointer %s",
                          text.empty() ? "" : "; ", left ? "Left" : "Right", left ? "right" : "left",
                          otherSame ? "memcmp equal" : "DIFFERS", left ? "left" : "right", differing, numSamples,
                          finite ? "" : " (NON-FINITE)", aloneSame ? "gives the same bits" : "gives OTHER bits");
        }

        const bool lanesOk = eq::Router::touchesLane (eq::Placement::left, 0, true)
                          && ! eq::Router::touchesLane (eq::Placement::left, 1, true)
                          && ! eq::Router::touchesLane (eq::Placement::right, 0, true)
                          && eq::Router::touchesLane (eq::Placement::right, 1, true)
                          && eq::Router::touchesLane (eq::Placement::mid, 0, true)
                          && eq::Router::touchesLane (eq::Placement::side, 1, true)
                          && eq::Router::touchesLane (eq::Placement::right, 0, false)
                          && ! eq::Router::touchesLane (eq::Placement::right, 1, false);

        row (ok && lanesOk, "EN-lr 6 Left bands (mixed types): right lane memcmp equal, left lane differs; and the converse",
             text + (lanesOk ? "; touchesLane agrees" : "; touchesLane DISAGREES"));
    }

    //==========================================================================
    // EN-g8: the output starts at the impulse index; first-sample gain
    //==========================================================================
    constexpr int kEnG8Random = 10000;
    constexpr int kEnG8Lane = 16;
    constexpr double kEnG8RelLimit = 1.0e-12;

    struct EnG8Config
    {
        int rateIndex = 0;              // into kRates5
        bool stereo = true;
        double impulseR = 1.0;          // the right lane's sample at index 0 (the left lane's is 1)
        float scale = 100.0f;
        int numBands = 0;
        std::array<eq::RawBand, kEnMaxBands> raw {};
    };

    /** Configuration `index` of the random set: its own generator, so the set
        does not depend on the order or the thread it is drawn in. 1 - 24
        bands, full range, gain scale 0 - 200 %, one of five rates; one in
        four on a mono layout; the impulse on both lanes or on the left alone. */
    EnG8Config enG8Draw (int index)
    {
        Rng seed (0x454E473800ull + static_cast<std::uint64_t> (index));
        Rng rng (seed.next());

        EnG8Config c;
        c.rateIndex = rng.below (kEnNumRates);
        c.stereo = rng.below (4) != 0;
        c.impulseR = rng.below (2) == 0 ? 1.0 : 0.0;
        c.scale = static_cast<float> (200.0 * rng.uniform());
        c.numBands = 1 + rng.below (kEnMaxBands);

        for (int b = 0; b < c.numBands; ++b)
            c.raw[ix (b)] = enRandomRaw (rng);

        return c;
    }

    struct EnG8Outcome
    {
        double predicted[2] = { 0.0, 0.0 };
        double actual[2] = { 0.0, 0.0 };
        int firstNonZero[2] = { -1, -1 };       // -1: the lane is zero throughout
        double rel = 0.0;                       // the larger of the lanes' |actual - predicted| / |predicted|
        double scaled = 0.0;                    // the larger of the lanes' |actual - predicted| / the chain's largest magnitude
        bool startOk = true;
        bool absorbed = false;                  // a lane reads exactly 0 at index 0, and so does the prediction
    };

    /** A unit impulse at index 0 through the configuration's bands in index
        order, every band at rest. The prediction is Router::applyFirstSample
        applied band by band to Band::firstSampleGain().

        Scoring as G8 was amended at the 2.2 execute (Taylor's call, 2026-11-13):
        the error |actual - predicted| is taken relative to the largest
        magnitude either lane's prediction reaches anywhere in the chain (at
        least 1, the impulse), because Mid / Side difference injection and the
        SVF's high-pass term subtract values of that size, and rounding in
        them is not relative to a small result. A lane passes the start test
        when its first non-zero sample is at index 0, or when its predicted
        first sample is exactly 0 (the same algebra absorbed it, or nothing
        was put on the lane). The plain relative error is still reported, and
        still binds the three forced single-band cases. */
    EnG8Outcome enG8Run (const eq::Designer& designer, const EnG8Config& c, double* lane0, double* lane1, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            lane0[i] = 0.0;
            lane1[i] = 0.0;
        }

        lane0[0] = 1.0;
        lane1[0] = c.impulseR;

        double l = 1.0, r = c.impulseR;
        double chainMag = 1.0;
        eq::BandDesign d {};
        eq::Band band;

        for (int b = 0; b < c.numBands; ++b)
        {
            const eq::BandInput in = eq::makeBandInput (c.raw[ix (b)], c.scale, 0.0);
            designer.design (in.settings, d);
            band.setDesign (d);
            band.reset();
            eq::Router::applyFirstSample (band.firstSampleGain(), in.placement, c.stereo, l, r);
            eq::Router::process (band, in.placement, lane0, lane1, numSamples, c.stereo);

            if (std::isfinite (l) && std::isfinite (r))
                chainMag = std::max (chainMag, std::max (std::fabs (l), c.stereo ? std::fabs (r) : 0.0));
        }

        EnG8Outcome out;
        out.predicted[0] = l;
        out.predicted[1] = r;

        const double* const lanes[2] = { lane0, lane1 };
        const double impulse[2] = { 1.0, c.impulseR };
        const int numLanes = c.stereo ? 2 : 1;

        for (int lane = 0; lane < numLanes; ++lane)
        {
            const double* const y = lanes[lane];
            out.actual[lane] = y[0];

            for (int i = 0; i < numSamples; ++i)
                if (y[i] != 0.0)
                {
                    out.firstNonZero[lane] = i;
                    break;
                }

            const double p = out.predicted[lane];
            const double a = out.actual[lane];
            double laneRel = 0.0;

            if (! std::isfinite (p) || ! std::isfinite (a))
                laneRel = kInf;
            else if (p == 0.0)
                laneRel = (a == 0.0) ? 0.0 : kInf;
            else
                laneRel = std::fabs (a - p) / std::fabs (p);

            out.rel = std::max (out.rel, laneRel);
            out.scaled = std::max (out.scaled, (std::isfinite (p) && std::isfinite (a)) ? std::fabs (a - p) / chainMag : kInf);

            const bool silentLane = impulse[lane] == 0.0 && out.firstNonZero[lane] < 0 && p == 0.0;

            if (out.firstNonZero[lane] != 0 && ! silentLane)
            {
                if (p == 0.0 && a == 0.0)
                    out.absorbed = true;        // the prediction is exactly 0 as well: excused (amended G8)
                else
                    out.startOk = false;
            }
        }

        return out;
    }

    /** The whole configuration, band by band, with the running prediction and
        the running output at index 0: what an escalation needs. */
    void enG8Describe (const char* label, int index, const EnG8Config& c)
    {
        eq::Designer designer;
        designer.prepare (kRates5[ix (c.rateIndex)]);

        note (strf ("%s: configuration %d, fs %.0f Hz, %s layout, %d bands, gain scale %.9g %%, impulse L 1, R %g", label, index,
                    kRates5[ix (c.rateIndex)], c.stereo ? "stereo" : "mono", c.numBands, static_cast<double> (c.scale), c.impulseR));

        double lane0[kEnG8Lane] = {}, lane1[kEnG8Lane] = {};
        lane0[0] = 1.0;
        lane1[0] = c.impulseR;

        double l = 1.0, r = c.impulseR;
        eq::BandDesign d {};
        eq::Band band;

        for (int b = 0; b < c.numBands; ++b)
        {
            const eq::BandInput in = eq::makeBandInput (c.raw[ix (b)], c.scale, 0.0);
            designer.design (in.settings, d);
            band.setDesign (d);
            band.reset();
            const double gain = band.firstSampleGain();
            eq::Router::applyFirstSample (gain, in.placement, c.stereo, l, r);
            eq::Router::process (band, in.placement, lane0, lane1, kEnG8Lane, c.stereo);

            note (strf ("  band %2d: %-9s slope-index %d fc %.9g Hz Q %.9g gain %.9g dB %-6s first-sample gain %.17g | "
                        "predicted L %.17g R %.17g | actual L %.17g R %.17g",
                        b, kTypeNames[static_cast<int> (in.settings.type)], in.settings.slopeIndex, in.settings.freqHz,
                        in.settings.q, in.settings.gainDbEff, kEnPlacementNames[static_cast<int> (in.placement)], gain,
                        l, r, lane0[0], lane1[0]));
        }
    }

    void rowEnG8()
    {
        constexpr int chunkSize = 100;
        constexpr int numChunks = kEnG8Random / chunkSize;

        struct Chunk
        {
            long long configs = 0, bands = 0, mono = 0, startBad = 0, absorbed = 0, relBad = 0, scaledBad = 0;
            double worstRel = 0.0, worstScaled = 0.0;
            int worstIndex = -1, firstStartBad = -1, firstRelBad = -1, worstScaledIndex = -1;
        };

        std::vector<Chunk> chunks (ix (numChunks));

        // Fixed chunks, per-configuration generators, merged in chunk order.
        parallelFor (numChunks, [&chunks] (int c)
        {
            std::array<eq::Designer, kEnNumRates> designers;      // this worker's own

            for (std::size_t r = 0; r < kRates5.size(); ++r)
                designers[r].prepare (kRates5[r]);

            Chunk& out = chunks[ix (c)];
            double lane0[kEnG8Lane], lane1[kEnG8Lane];

            for (int n = c * chunkSize; n < (c + 1) * chunkSize; ++n)
            {
                const EnG8Config config = enG8Draw (n);
                const EnG8Outcome o = enG8Run (designers[ix (config.rateIndex)], config, lane0, lane1, kEnG8Lane);
                ++out.configs;
                out.bands += config.numBands;

                if (! config.stereo)
                    ++out.mono;

                if (o.absorbed)
                    ++out.absorbed;

                if (! o.startOk)
                {
                    ++out.startBad;

                    if (out.firstStartBad < 0)
                        out.firstStartBad = n;
                }

                if (! (o.scaled <= kEnG8RelLimit))
                    ++out.scaledBad;

                if (! (o.scaled <= out.worstScaled))
                {
                    out.worstScaled = o.scaled;
                    out.worstScaledIndex = n;
                }

                if (! (o.rel <= kEnG8RelLimit))
                {
                    ++out.relBad;

                    if (out.firstRelBad < 0)
                        out.firstRelBad = n;
                }

                if (o.rel > out.worstRel)
                {
                    out.worstRel = o.rel;
                    out.worstIndex = n;
                }
            }
        });

        Chunk sum;

        for (const Chunk& r : chunks)
        {
            sum.configs += r.configs;
            sum.bands += r.bands;
            sum.mono += r.mono;
            sum.startBad += r.startBad;
            sum.absorbed += r.absorbed;
            sum.relBad += r.relBad;
            sum.scaledBad += r.scaledBad;

            if (! (r.worstScaled <= sum.worstScaled))
            {
                sum.worstScaled = r.worstScaled;
                sum.worstScaledIndex = r.worstScaledIndex;
            }

            if (r.worstRel > sum.worstRel)
            {
                sum.worstRel = r.worstRel;
                sum.worstIndex = r.worstIndex;
            }

            if (sum.firstStartBad < 0)
                sum.firstStartBad = r.firstStartBad;

            if (sum.firstRelBad < 0)
                sum.firstRelBad = r.firstRelBad;
        }

        // The three forced cases of RESEARCH-2.2 Q3: one High Cut, 96 dB/oct, Q 0.025, mono layout.
        struct Forced { int rateIndex; double freqHz; const char* research; };
        static const Forced forced[3] = { { 0, 10.0, "2.0e-47" }, { 4, 10.0, "1.2e-57" }, { 4, 100.0, "1.2e-41" } };
        constexpr int forcedLength = 4096;
        std::vector<double> long0 (ix (forcedLength)), long1 (ix (forcedLength));
        std::vector<std::string> forcedLines;
        int forcedBad = 0;

        for (const Forced& f : forced)
        {
            EnG8Config config;
            config.rateIndex = f.rateIndex;
            config.stereo = false;
            config.impulseR = 0.0;
            config.scale = kEnScale100;
            config.numBands = 1;
            config.raw[0] = enRaw (eq::Type::highCut, f.freqHz, 0.0, 0.025, 7, eq::Placement::stereo);

            eq::Designer designer;
            designer.prepare (kRates5[ix (f.rateIndex)]);
            const EnG8Outcome o = enG8Run (designer, config, long0.data(), long1.data(), forcedLength);
            const bool caseOk = o.startOk && o.rel <= kEnG8RelLimit;

            if (! caseOk)
                ++forcedBad;

            forcedLines.push_back (strf ("forced High Cut %g Hz, 96 dB/oct, Q 0.025 at %.0f Hz: first non-zero index %d, y[0] %.6e "
                                         "(predicted %.6e, relative error %.3g; research %s)%s",
                                         f.freqHz, kRates5[ix (f.rateIndex)], o.firstNonZero[0], o.actual[0], o.predicted[0], o.rel,
                                         f.research, caseOk ? "" : "  OFF"));
        }

        // Vacuity: the scaled error must be a live measurement (rounding makes it non-zero somewhere).
        const bool live = sum.worstScaled > 0.0;
        const bool ok = sum.configs == kEnG8Random && sum.startBad == 0 && sum.scaledBad == 0 && forcedBad == 0 && live;

        row (ok, "EN-g8 unit impulse, 10 000 random configurations + 3 forced High Cuts: output starts at the impulse index "
                 "wherever the predicted y[0] is non-zero; |y[0] - predicted| <= 1e-12 x the chain's largest magnitude "
                 "(G8 as amended at the 2.2 execute); forced cases 1e-12 relative",
             strf ("%lld random configurations (%lld bands, %lld on a mono layout): %lld do not start at the impulse index with a "
                   "non-zero prediction; %lld above 1e-12 re the chain's largest magnitude, worst %.3g (configuration %d); "
                   "forced cases off: %d of 3. Not gated: %lld configurations with y[0] and its prediction both exactly 0 "
                   "(absorbed by Mid / Side injection), %lld above 1e-12 relative to the prediction itself (cancellation)",
                   sum.configs, sum.bands, sum.mono, sum.startBad, sum.scaledBad, sum.worstScaled, sum.worstScaledIndex, forcedBad,
                   sum.absorbed, sum.relBad));

        for (const std::string& text : forcedLines)
            note (text);

        if (sum.scaledBad > 0 && sum.worstScaledIndex >= 0)
            enG8Describe ("worst |y[0] - predicted| / chain magnitude", sum.worstScaledIndex, enG8Draw (sum.worstScaledIndex));

        if (sum.firstStartBad >= 0)
            enG8Describe ("first that does not start at the impulse index", sum.firstStartBad, enG8Draw (sum.firstStartBad));
    }

    //==========================================================================
    // EN-blk: block-size invariance
    //==========================================================================
    struct EnBlkResult
    {
        int mismatches = 0;         // block sizes whose output differs from the block-size-1 render
        bool changed = false;       // the reference render differs from the input
        bool finite = true;
    };

    /** Stereo noise, 48 kHz, 65 536 samples, at block sizes 1, 7, 16, 32, 64,
        512, 2048, 4096 and one block. Every size against the size-1 render. */
    EnBlkResult enBlkRun (const std::vector<eq::BandInput>& bandInputs)
    {
        constexpr int numSamples = 1 << 16;
        static const int sizes[9] = { 1, 7, 16, 32, 64, 512, 2048, 4096, numSamples };

        eq::Designer designer;
        designer.prepare (48000.0);

        Rng rng (0x454E424Cull);
        std::vector<double> inL (ix (numSamples)), inR (ix (numSamples));
        enFillNoise (inL, rng);
        enFillNoise (inR, rng);

        const std::size_t bytes = sizeof (double) * ix (numSamples);
        std::vector<double> refL, refR;
        EnBlkResult result;

        for (const int size : sizes)
        {
            EnChain chain;

            for (const eq::BandInput& in : bandInputs)
                chain.add (designer, in);

            std::vector<double> outL (inL), outR (inR);

            for (int pos = 0; pos < numSamples; pos += size)
            {
                const int n = std::min (size, numSamples - pos);
                chain.process (outL.data() + pos, outR.data() + pos, n, true);
            }

            if (refL.empty())
            {
                refL = outL;
                refR = outR;
                continue;
            }

            if (std::memcmp (refL.data(), outL.data(), bytes) != 0 || std::memcmp (refR.data(), outR.data(), bytes) != 0)
                ++result.mismatches;
        }

        result.changed = std::memcmp (refL.data(), inL.data(), bytes) != 0 && std::memcmp (refR.data(), inR.data(), bytes) != 0;

        for (int i = 0; i < numSamples; ++i)
            if (! std::isfinite (refL[ix (i)]) || ! std::isfinite (refR[ix (i)]))
                result.finite = false;

        return result;
    }

    void rowEnBlk()
    {
        // (a) a 96 dB/oct High Shelf into a 36 dB/oct Low Cut, both Stereo.
        std::vector<eq::BandInput> pair;
        pair.push_back (eq::makeBandInput (enRaw (eq::Type::highShelf, 6000.0, 9.0, 2.0, 7, eq::Placement::stereo), kEnScale100, 0.0));
        pair.push_back (eq::makeBandInput (enRaw (eq::Type::lowCut, 40.0, 0.0, 1.0, 4, eq::Placement::stereo), kEnScale100, 0.0));

        // (b) 24 bands: every type three times, mixed slopes, every placement.
        std::vector<eq::BandInput> mixed;

        for (int b = 0; b < kEnMaxBands; ++b)
        {
            const eq::Type type = static_cast<eq::Type> (b % 8);
            const eq::Placement placement = static_cast<eq::Placement> (b % 5);
            const int slopeIndex = (b * 3 + 1) % 8;
            const double fc = 30.0 * std::pow (600.0, static_cast<double> (b) / 23.0);
            const double gain = (b % 2 == 0 ? 1.0 : -1.0) * (2.0 + static_cast<double> (b % 4));
            const double q = 0.5 * std::pow (8.0, static_cast<double> ((b * 7) % 24) / 23.0);
            mixed.push_back (eq::makeBandInput (enRaw (type, fc, gain, q, slopeIndex, placement), kEnScale100, 0.0));
        }

        const EnBlkResult a = enBlkRun (pair);
        const EnBlkResult b = enBlkRun (mixed);
        const bool ok = a.mismatches == 0 && b.mismatches == 0 && a.changed && b.changed && a.finite && b.finite;

        row (ok, "EN-blk block sizes 1, 7, 16, 32, 64, 512, 2048, 4096, one block: bit-identical (stereo noise, 48 kHz, 65 536 samples)",
             strf ("High Shelf 96 dB/oct into Low Cut 36 dB/oct: %d of 8 sizes differ from the size-1 render%s%s; "
                   "24 bands, mixed types / slopes / placements: %d of 8 differ%s%s",
                   a.mismatches, a.changed ? "" : " (VOID: output == input)", a.finite ? "" : " (NON-FINITE)",
                   b.mismatches, b.changed ? "" : " (VOID: output == input)", b.finite ? "" : " (NON-FINITE)"));
    }

    //==========================================================================
    // EN-fin: every sample finite
    //==========================================================================
    struct EnFinAbove
    {
        long long configs = 0, samples = 0, nonFinite = 0;
        std::string first;
    };

    /** (i) Every type with its frequency at 20, 21 ... 30 kHz at 44.1 kHz:
        every slope (slope types), Q 0.025 / 1 / 40, gain +-30 dB at scale 100
        and 200 % (gain types). 2048 samples of noise, then 2048 of silence. */
    EnFinAbove enFinAboveNyquist()
    {
        constexpr int half = 2048;
        static const double qs[3] = { 0.025, 1.0, 40.0 };
        static const double gains[2] = { -30.0, 30.0 };
        static const float scales[2] = { 100.0f, 200.0f };

        const double fs = 44100.0;
        eq::Designer designer;
        designer.prepare (fs);

        Rng rng (0x454E4631ull);
        std::vector<double> noise (ix (half));
        enFillNoise (noise, rng);

        std::vector<double> lane (ix (2 * half));
        eq::BandDesign d {};
        eq::Band band;
        EnFinAbove out;

        for (int typeIndex = 0; typeIndex < 8; ++typeIndex)
        {
            const eq::Type type = static_cast<eq::Type> (typeIndex);

            for (int slopeIndex = 0; slopeIndex < 8; ++slopeIndex)
            {
                if (! eq::usesSlope (type) && slopeIndex != 0)
                    continue;       // the canonical form: one design for every slope

                for (int step = 0; step <= 10; ++step)
                    for (const double q : qs)
                        for (int gi = 0; gi < 2; ++gi)
                            for (int si = 0; si < 2; ++si)
                            {
                                if (! eq::isGainType (type) && (gi != 0 || si != 0))
                                    continue;       // gain is not read

                                const double fc = 20000.0 + 1000.0 * static_cast<double> (step);
                                const eq::BandInput in = eq::makeBandInput (enRaw (type, fc, gains[gi], q, slopeIndex, eq::Placement::stereo),
                                                                            scales[si], 0.0);
                                designer.design (in.settings, d);
                                band.setDesign (d);
                                band.reset();

                                std::fill (lane.begin(), lane.end(), 0.0);
                                std::copy (noise.begin(), noise.end(), lane.begin());
                                eq::Router::process (band, in.placement, lane.data(), nullptr, 2 * half, false);
                                ++out.configs;

                                for (const double v : lane)
                                {
                                    ++out.samples;

                                    if (! std::isfinite (v))
                                    {
                                        ++out.nonFinite;

                                        if (out.first.empty())
                                            out.first = strf ("%s slope-index %d fc %.0f Hz Q %g gain %g dB (effective), 44100 Hz",
                                                              kTypeNames[typeIndex], in.settings.slopeIndex, fc, q, in.settings.gainDbEff);
                                    }
                                }
                            }
            }
        }

        return out;
    }

    struct EnFinRate
    {
        long long frames = 0, nonFinite = 0, structural = 0, continuous = 0, firstBad = -1;
        double peak = 0.0;
    };

    /** (ii) 10 s at one rate, stereo layout, 24 active bands. Every 64 samples
        every band is re-drawn over the full range and the gain scale over
        0 - 200 %; half of the re-draws keep the band's type, slope and
        placement. The 2.2 processor's rule is applied: a structural change
        clears the band's state, a continuous one keeps it. Noise at -12 dBFS
        and silence alternate every 0.5 s. Designer::prepare for this rate. */
    EnFinRate enFinRandom (int rateIndex)
    {
        constexpr int block = 64;

        const double fs = kRates8[ix (rateIndex)];
        const long long total = std::llround (10.0 * fs);
        const long long segment = std::llround (0.5 * fs);

        eq::Designer designer;
        designer.prepare (fs);

        Rng seed (0x454E463200ull + static_cast<std::uint64_t> (rateIndex));
        Rng rng (seed.next());

        std::array<eq::Band, kEnMaxBands> bands;
        std::array<eq::BandInput, kEnMaxBands> current {};
        std::array<eq::RawBand, kEnMaxBands> raws {};
        eq::BandDesign d {};
        double lane0[block], lane1[block];
        bool firstBlock = true;
        EnFinRate out;

        for (long long pos = 0; pos < total; pos += block)
        {
            const int n = static_cast<int> (std::min (static_cast<long long> (block), total - pos));
            const float scale = static_cast<float> (200.0 * rng.uniform());

            for (int b = 0; b < kEnMaxBands; ++b)
            {
                eq::RawBand next = enRandomRaw (rng);
                const bool keepStructure = rng.below (2) == 0;

                if (keepStructure && ! firstBlock)
                {
                    next.type = raws[ix (b)].type;
                    next.slope = raws[ix (b)].slope;
                    next.placement = raws[ix (b)].placement;
                }

                raws[ix (b)] = next;

                const eq::BandInput in = eq::makeBandInput (next, scale, 0.0);
                const eq::BandChange change = firstBlock ? eq::BandChange::structural
                                                         : eq::classifyBandChange (current[ix (b)], in);

                if (change != eq::BandChange::identical)
                {
                    designer.design (in.settings, d);
                    bands[ix (b)].setDesign (d);

                    if (change == eq::BandChange::structural)
                    {
                        bands[ix (b)].reset();
                        ++out.structural;
                    }
                    else
                    {
                        ++out.continuous;
                    }
                }

                current[ix (b)] = in;
            }

            firstBlock = false;

            const bool noisy = ((pos / segment) % 2) == 0;

            for (int i = 0; i < n; ++i)
            {
                lane0[i] = noisy ? (rng.uniform() * 2.0 - 1.0) * 0.25 : 0.0;
                lane1[i] = noisy ? (rng.uniform() * 2.0 - 1.0) * 0.25 : 0.0;
            }

            for (int b = 0; b < kEnMaxBands; ++b)
                eq::Router::process (bands[ix (b)], current[ix (b)].placement, lane0, lane1, n, true);

            for (int i = 0; i < n; ++i)
            {
                ++out.frames;

                if (! std::isfinite (lane0[i]) || ! std::isfinite (lane1[i]))
                {
                    ++out.nonFinite;

                    if (out.firstBad < 0)
                        out.firstBad = pos + i;

                    continue;
                }

                out.peak = std::max (out.peak, std::max (std::fabs (lane0[i]), std::fabs (lane1[i])));
            }
        }

        return out;
    }

    void rowEnFin()
    {
        const EnFinAbove above = enFinAboveNyquist();

        std::array<EnFinRate, kEnNumRates8> rates;

        parallelFor (kEnNumRates8, [&rates] (int r)
        {
            rates[ix (r)] = enFinRandom (r);
        });

        long long frames = 0, nonFinite = 0;
        bool framesOk = true;
        std::string first;

        for (int r = 0; r < kEnNumRates8; ++r)
        {
            const EnFinRate& rate = rates[ix (r)];
            frames += rate.frames;
            nonFinite += rate.nonFinite;
            framesOk = framesOk && rate.frames == std::llround (10.0 * kRates8[ix (r)]) && rate.structural > 0 && rate.continuous > 0;

            if (first.empty() && rate.firstBad >= 0)
                first = strf ("%.0f Hz, frame %lld", kRates8[ix (r)], rate.firstBad);
        }

        row (above.nonFinite == 0 && above.configs > 0 && nonFinite == 0 && framesOk,
             "EN-fin every sample finite: (i) every type at 20-30 kHz, 44.1 kHz; (ii) random full-range settings re-drawn every "
             "64 samples, 8 rates x 10 s",
             strf ("(i) %lld settings, %lld samples, %lld non-finite%s%s; (ii) %lld stereo frames, %lld non-finite%s%s%s",
                   above.configs, above.samples, above.nonFinite, above.first.empty() ? "" : ", first: ", above.first.c_str(),
                   frames, nonFinite, first.empty() ? "" : ", first at ", first.c_str(),
                   framesOk ? "" : " (VOID: a rate is short of 10 s or saw no structural / continuous change)"));

        for (int r = 0; r < kEnNumRates8; ++r)
        {
            const EnFinRate& rate = rates[ix (r)];
            note (strf ("(ii) %6.0f Hz: %lld frames, 24 bands, %lld structural changes (state cleared), %lld continuous (state kept), "
                        "%lld non-finite, peak %+.1f dB re the noise peak",
                        kRates8[ix (r)], rate.frames, rate.structural, rate.continuous, rate.nonFinite,
                        20.0 * std::log10 (std::max (rate.peak, kTiny) / 0.25)));
        }
    }

    //==========================================================================
    // EN-alloc: allocation in the engine
    //==========================================================================
    /** 1e4 blocks inside a malloc_logger window on this thread: each block
        designs one band (Designer::design, Band::setDesign, the state cleared
        on a structural change) and runs all 24 through the Router on a
        64-sample stereo block. `plant` puts one deliberate allocation inside
        the window (the negative control). The hook, its counters and the
        vacuity probe are row A1's. */
    AllocResult runEnAlloc (bool plant)
    {
        AllocResult result;

       #if defined (__APPLE__)
        constexpr int numBlocks = 10000;
        constexpr int blockSize = 64;
        constexpr int noiseLength = 4096;

        eq::Designer designer;
        designer.prepare (48000.0);

        Rng rng (0x454E414Cull);
        std::vector<eq::BandInput> inputs;
        inputs.reserve (ix (numBlocks));

        for (int i = 0; i < numBlocks; ++i)
            inputs.push_back (eq::makeBandInput (enRandomRaw (rng), kEnScale100, 0.0));

        std::vector<double> noiseL (ix (noiseLength)), noiseR (ix (noiseLength));
        enFillNoise (noiseL, rng);
        enFillNoise (noiseR, rng);

        std::array<eq::Band, kEnMaxBands> bands;
        std::array<eq::BandInput, kEnMaxBands> current {};
        eq::BandDesign d {};

        for (int b = 0; b < kEnMaxBands; ++b)
        {
            current[ix (b)] = eq::makeBandInput (enRandomRaw (rng), kEnScale100, 0.0);
            designer.design (current[ix (b)].settings, d);
            bands[ix (b)].setDesign (d);
            bands[ix (b)].reset();
        }

        double lane0[blockSize], lane1[blockSize];
        long long finiteBlocks = 0;

        malloc_logger_t* const previous = malloc_logger;
        allocThread = pthread_self();
        allocCount = 0;
        freeCount = 0;
        malloc_logger = countAllocation;

        // Vacuity: the hook must see a deliberate allocation and its release.
        allocArmed = true;
        {
            void* volatile planted = std::malloc (64);
            std::free (planted);
        }
        allocArmed = false;
        result.vacuityAllocs = allocCount;
        result.vacuityFrees = freeCount;
        allocCount = 0;
        freeCount = 0;

        allocArmed = true;

        for (int i = 0; i < numBlocks; ++i)
        {
            const int b = i % kEnMaxBands;
            const eq::BandInput& in = inputs[ix (i)];
            designer.design (in.settings, d);
            bands[ix (b)].setDesign (d);

            if (eq::classifyBandChange (current[ix (b)], in) == eq::BandChange::structural)
                bands[ix (b)].reset();

            current[ix (b)] = in;

            const std::size_t offset = ix ((i * blockSize) % (noiseLength - blockSize));
            std::memcpy (lane0, noiseL.data() + offset, sizeof (lane0));
            std::memcpy (lane1, noiseR.data() + offset, sizeof (lane1));

            for (int k = 0; k < kEnMaxBands; ++k)
                eq::Router::process (bands[ix (k)], current[ix (k)].placement, lane0, lane1, blockSize, true);

            if (std::isfinite (lane0[blockSize - 1]) && std::isfinite (lane1[blockSize - 1]))
                ++finiteBlocks;
        }

        if (plant)
        {
            void* volatile planted = std::malloc (64);
            std::free (planted);
        }

        allocArmed = false;
        malloc_logger = previous;

        result.available = true;
        result.allocs = allocCount;
        result.frees = freeCount;
        result.checksum = finiteBlocks;
       #else
        (void) plant;
       #endif

        return result;
    }

    void rowEnAlloc()
    {
        const AllocResult r = runEnAlloc (false);

        if (! r.available)
        {
            info ("EN-alloc allocation gate SKIPPED: no malloc_logger on this platform");
            return;
        }

        const bool hookLive = r.vacuityAllocs >= 1 && r.vacuityFrees >= 1;
        row (hookLive && r.allocs == 0 && r.frees == 0,
             "EN-alloc 1e4 blocks of design + setDesign + Router over 24 bands: 0 allocations, 0 frees (malloc_logger, this thread)",
             strf ("%d allocations, %d frees; hook %s (planted: %d / %d); %lld of 10000 blocks end finite",
                   r.allocs, r.frees, hookLive ? "live" : "DEAD", r.vacuityAllocs, r.vacuityFrees, r.checksum));
    }

    //==========================================================================
    // SN1 - SN3: makeBandInput (PLAN-2.2 P2)
    //==========================================================================
    void rowEnSn1()
    {
        int checks = 0, bad = 0;
        std::string first;

        const auto expect = [&checks, &bad, &first] (bool ok, const char* what)
        {
            ++checks;

            if (! ok)
            {
                ++bad;

                if (first.empty())
                    first = what;
            }
        };

        const float quietNaN = std::numeric_limits<float>::quiet_NaN();
        const float infinity = std::numeric_limits<float>::infinity();
        const float nonFinite[3] = { quietNaN, infinity, -infinity };

        // A Low Shelf, 24 dB/oct, on Right: no canonical rule touches it.
        const eq::RawBand base = enRaw (eq::Type::lowShelf, 1234.5, 7.25, 2.5, 3, eq::Placement::right);

        {
            const eq::BandInput in = eq::makeBandInput (base, 100.0f, 0.0);
            expect (in.settings.type == eq::Type::lowShelf, "base: type");
            expect (sameBits (in.settings.freqHz, 1234.5), "base: frequency is the widened float");
            expect (sameBits (in.settings.gainDbEff, 7.25), "base: gain");
            expect (sameBits (in.settings.q, 2.5), "base: Q");
            expect (in.settings.slopeIndex == 3, "base: slope index");
            expect (in.placement == eq::Placement::right, "base: placement");
            expect (in.active, "base: active");
        }

        // 1. Non-finite -> the default.
        for (const float v : nonFinite)
        {
            eq::RawBand raw = base;
            raw.enabled = v;
            expect (! eq::makeBandInput (raw, 100.0f, 0.0).active, "non-finite enabled reads off");

            raw = base;
            raw.bypass = v;
            expect (eq::makeBandInput (raw, 100.0f, 0.0).active, "non-finite bypass reads off");

            raw = base;
            raw.type = v;
            expect (eq::makeBandInput (raw, 100.0f, 0.0).settings.type == eq::Type::bell, "non-finite type reads 0 (Bell)");

            raw = base;
            raw.freq = v;
            expect (sameBits (eq::makeBandInput (raw, 100.0f, 0.0).settings.freqHz, 1000.0), "non-finite frequency reads 1000 Hz");

            raw = base;
            raw.gain = v;
            expect (eq::makeBandInput (raw, 100.0f, 0.0).settings.gainDbEff == 0.0, "non-finite gain reads 0 dB");

            raw = base;
            raw.q = v;
            expect (sameBits (eq::makeBandInput (raw, 100.0f, 0.0).settings.q, 1.0), "non-finite Q reads 1");

            raw = base;
            raw.slope = v;
            expect (eq::makeBandInput (raw, 100.0f, 0.0).settings.slopeIndex == 1, "non-finite slope reads index 1");

            raw = base;
            raw.placement = v;
            expect (eq::makeBandInput (raw, 100.0f, 0.0).placement == eq::Placement::stereo, "non-finite placement reads 0 (Stereo)");

            expect (sameBits (eq::makeBandInput (base, v, 0.0).settings.gainDbEff, 7.25), "non-finite gain scale reads 100 %");
            expect (sameBits (eq::makeBandInput (base, 100.0f, static_cast<double> (v)).settings.gainDbEff, 7.25),
                    "non-finite dynamic offset reads 0 dB");
        }

        {
            eq::RawBand raw;
            raw.enabled = quietNaN;  raw.bypass = quietNaN;  raw.type = quietNaN;       raw.freq = quietNaN;
            raw.gain = quietNaN;     raw.q = quietNaN;       raw.slope = quietNaN;      raw.placement = quietNaN;
            const eq::BandInput in = eq::makeBandInput (raw, quietNaN, 0.0);
            expect (! in.active && in.settings.type == eq::Type::bell && sameBits (in.settings.freqHz, 1000.0)
                      && in.settings.gainDbEff == 0.0 && sameBits (in.settings.q, 1.0) && in.settings.slopeIndex == 0
                      && in.placement == eq::Placement::stereo,
                    "all NaN: an inactive default Bell (canonical slope index 0)");
        }

        // 2. Clamps.
        {
            eq::RawBand raw = base;
            raw.freq = 5.0f;
            expect (sameBits (eq::makeBandInput (raw, 100.0f, 0.0).settings.freqHz, 10.0), "frequency 5 Hz clamps to 10");
            raw.freq = 40000.0f;
            expect (sameBits (eq::makeBandInput (raw, 100.0f, 0.0).settings.freqHz, 30000.0), "frequency 40 kHz clamps to 30 kHz");

            raw = base;
            raw.gain = 45.0f;
            expect (sameBits (eq::makeBandInput (raw, 100.0f, 0.0).settings.gainDbEff, 30.0), "gain +45 dB clamps to +30");
            raw.gain = -45.0f;
            expect (sameBits (eq::makeBandInput (raw, 100.0f, 0.0).settings.gainDbEff, -30.0), "gain -45 dB clamps to -30");

            raw = base;
            raw.q = 0.001f;
            expect (sameBits (eq::makeBandInput (raw, 100.0f, 0.0).settings.q, 0.025), "Q 0.001 clamps to 0.025");
            raw.q = 100.0f;
            expect (sameBits (eq::makeBandInput (raw, 100.0f, 0.0).settings.q, 40.0), "Q 100 clamps to 40");
            raw.q = 0.025f;
            expect (sameBits (eq::makeBandInput (raw, 100.0f, 0.0).settings.q, static_cast<double> (0.025f)),
                    "Q 0.025f (in range) is the widened float");

            expect (eq::makeBandInput (base, -10.0f, 0.0).settings.gainDbEff == 0.0, "gain scale -10 % clamps to 0");
            expect (sameBits (eq::makeBandInput (base, 250.0f, 0.0).settings.gainDbEff, 14.5), "gain scale 250 % clamps to 200");
        }

        // 2. Choices: clamp to the index range, round to nearest, a half up.
        {
            struct Choice { float raw; int expected; };
            static const Choice types[8] = { { -3.0f, 0 }, { 12.0f, 7 }, { 2.4f, 2 }, { 2.5f, 3 }, { 2.6f, 3 }, { 6.5f, 7 }, { 7.4f, 7 }, { 0.49f, 0 } };
            static const Choice slopes[7] = { { -1.0f, 0 }, { 99.0f, 7 }, { 3.49f, 3 }, { 3.5f, 4 }, { 0.5f, 1 }, { 6.51f, 7 }, { 7.0f, 7 } };
            static const Choice placements[7] = { { -2.0f, 0 }, { 9.0f, 4 }, { 1.5f, 2 }, { 0.49f, 0 }, { 3.5f, 4 }, { 2.49f, 2 }, { 4.0f, 4 } };

            for (const Choice& c : types)
            {
                eq::RawBand raw = base;
                raw.type = c.raw;
                expect (static_cast<int> (eq::makeBandInput (raw, 100.0f, 0.0).settings.type) == c.expected, "type choice rounding / clamp");
            }

            for (const Choice& c : slopes)
            {
                eq::RawBand raw = base;
                raw.slope = c.raw;
                expect (eq::makeBandInput (raw, 100.0f, 0.0).settings.slopeIndex == c.expected, "slope choice rounding / clamp");
            }

            for (const Choice& c : placements)
            {
                eq::RawBand raw = base;
                raw.placement = c.raw;
                expect (static_cast<int> (eq::makeBandInput (raw, 100.0f, 0.0).placement) == c.expected, "placement choice rounding / clamp");
            }
        }

        // 2, 4. Toggles: on from 0.5; active = enabled && ! bypass.
        {
            eq::RawBand raw = base;
            raw.enabled = 0.49f;
            expect (! eq::makeBandInput (raw, 100.0f, 0.0).active, "enabled 0.49 reads off");
            raw.enabled = 0.5f;
            expect (eq::makeBandInput (raw, 100.0f, 0.0).active, "enabled 0.5 reads on");

            raw = base;
            raw.bypass = 0.49f;
            expect (eq::makeBandInput (raw, 100.0f, 0.0).active, "bypass 0.49 reads off");
            raw.bypass = 0.5f;
            expect (! eq::makeBandInput (raw, 100.0f, 0.0).active, "bypass 0.5 reads on");

            for (int e = 0; e < 2; ++e)
                for (int b = 0; b < 2; ++b)
                {
                    raw = base;
                    raw.enabled = static_cast<float> (e);
                    raw.bypass = static_cast<float> (b);
                    expect (eq::makeBandInput (raw, 100.0f, 0.0).active == (e == 1 && b == 0), "active = enabled && ! bypass");
                }
        }

        // 3. Effective gain: scale 0, 100, 200; the +-60 dB clamp.
        {
            eq::RawBand raw = base;
            raw.gain = 7.3f;
            const double g = static_cast<double> (7.3f);
            expect (eq::makeBandInput (raw, 0.0f, 0.0).settings.gainDbEff == 0.0, "scale 0 %: 0 dB");
            expect (sameBits (eq::makeBandInput (raw, 100.0f, 0.0).settings.gainDbEff, g), "scale 100 %: the float gain, bit for bit");
            expect (sameBits (eq::makeBandInput (raw, 200.0f, 0.0).settings.gainDbEff, 2.0 * g), "scale 200 %: twice the float gain");

            raw.gain = -7.3f;
            expect (eq::makeBandInput (raw, 0.0f, 0.0).settings.gainDbEff == 0.0, "scale 0 %, negative gain: 0 dB");
            expect (sameBits (eq::makeBandInput (raw, 200.0f, 0.0).settings.gainDbEff, -2.0 * g), "scale 200 %, negative gain");

            raw.gain = 30.0f;
            expect (sameBits (eq::makeBandInput (raw, 200.0f, 0.0).settings.gainDbEff, 60.0), "+30 dB at 200 %: +60");
            expect (sameBits (eq::makeBandInput (raw, 200.0f, 5.0).settings.gainDbEff, 60.0), "+30 dB at 200 % + 5 dB offset clamps to +60");
            expect (sameBits (eq::makeBandInput (raw, 100.0f, 40.0).settings.gainDbEff, 60.0), "+30 dB + 40 dB offset clamps to +60");

            raw.gain = -30.0f;
            expect (sameBits (eq::makeBandInput (raw, 200.0f, 0.0).settings.gainDbEff, -60.0), "-30 dB at 200 %: -60");
            expect (sameBits (eq::makeBandInput (raw, 200.0f, -5.0).settings.gainDbEff, -60.0), "-30 dB at 200 % - 5 dB offset clamps to -60");

            raw.gain = 0.0f;
            expect (sameBits (eq::makeBandInput (raw, 100.0f, 59.5).settings.gainDbEff, 59.5), "0 dB + 59.5 dB offset: 59.5 (inside the clamp)");
            expect (sameBits (eq::makeBandInput (raw, 100.0f, -75.0).settings.gainDbEff, -60.0), "0 dB - 75 dB offset clamps to -60");
        }

        row (bad == 0 && checks >= 80,
             "SN1 makeBandInput: NaN / +-inf, out-of-range and off-grid values per field; toggles at 0.49 / 0.5; scale 0 / 100 / 200; +-60 clamp",
             strf ("%d checks, %d off%s%s", checks, bad, first.empty() ? "" : "; first: ", first.c_str()));
    }

    void rowEnSn2()
    {
        constexpr int numGains = 2000000;

        Rng rng (0x534E3200ull);
        eq::RawBand raw = enRaw (eq::Type::bell, 1000.0, 0.0, 1.0, 1, eq::Placement::stereo);
        long long bad = 0, nonZero = 0;
        double firstBad = 0.0;

        const auto check = [&raw, &bad, &firstBad] (float g)
        {
            raw.gain = g;
            const double eff = eq::makeBandInput (raw, kEnScale100, 0.0).settings.gainDbEff;

            if (! sameBits (eff, static_cast<double> (g)))
            {
                if (bad == 0)
                    firstBad = static_cast<double> (g);

                ++bad;
            }
        };

        for (int i = 0; i < numGains; ++i)
        {
            const float g = static_cast<float> (60.0 * rng.uniform() - 30.0);

            if (g != 0.0f)
                ++nonZero;

            check (g);
        }

        for (const float g : { -30.0f, 0.0f, 30.0f })
            check (g);

        row (bad == 0 && nonZero > numGains / 2,
             "SN2 2 000 000 random float gains (+-30 dB) at scale 100 %, offset 0: gainDbEff == the float gain, bit for bit",
             bad == 0 ? strf ("%d gains + the range ends, 0 differ", numGains)
                      : strf ("%lld of %d differ; first: %.9g dB", bad, numGains, firstBad));
    }

    void rowEnSn3()
    {
        constexpr int numDraws = 2000;
        static const eq::Type fixedTypes[3] = { eq::Type::bell, eq::Type::notch, eq::Type::bandPass };
        static const eq::Type slopeTypes[5] = { eq::Type::lowShelf, eq::Type::highShelf, eq::Type::lowCut, eq::Type::highCut, eq::Type::tilt };
        static const float qValues[5] = { 0.025f, 0.3f, 2.0f, 7.0f, 40.0f };

        Rng rng (0x534E3300ull);
        long long slopeEdits = 0, slopeBad = 0, qEdits = 0, qBad = 0, controls = 0, controlsFired = 0;

        for (int n = 0; n < numDraws; ++n)
        {
            eq::RawBand raw = enRandomRaw (rng);
            const float scale = static_cast<float> (200.0 * rng.uniform());

            // D6: the slope of a Bell / Notch / Band Pass.
            raw.type = static_cast<float> (static_cast<int> (fixedTypes[n % 3]));
            const eq::BandInput a = eq::makeBandInput (raw, scale, 0.0);

            for (int s = 0; s < 8; ++s)
            {
                eq::RawBand edited = raw;
                edited.slope = static_cast<float> (s);
                const eq::BandInput b = eq::makeBandInput (edited, scale, 0.0);
                ++slopeEdits;

                if (! (enSameFields (a, b) && b.settings.slopeIndex == 0 && eq::sameBandInput (a, b)
                         && eq::classifyBandChange (a, b) == eq::BandChange::identical))
                    ++slopeBad;
            }

            // D9: Q at 6 dB/oct, the five slope types.
            raw.type = static_cast<float> (static_cast<int> (slopeTypes[n % 5]));
            raw.slope = 0.0f;
            const eq::BandInput c = eq::makeBandInput (raw, scale, 0.0);

            for (const float q : qValues)
            {
                eq::RawBand edited = raw;
                edited.q = q;
                const eq::BandInput e = eq::makeBandInput (edited, scale, 0.0);
                ++qEdits;

                if (! (enSameFields (c, e) && sameBits (e.settings.q, 1.0) && eq::sameBandInput (c, e)
                         && eq::classifyBandChange (c, e) == eq::BandChange::identical))
                    ++qBad;
            }

            // Controls: the same edits where they are not inert.
            {
                raw.type = static_cast<float> (static_cast<int> (eq::Type::lowCut));
                raw.slope = 1.0f;
                eq::RawBand edited = raw;
                edited.slope = 5.0f;
                const eq::BandInput before = eq::makeBandInput (raw, scale, 0.0);
                const eq::BandInput after = eq::makeBandInput (edited, scale, 0.0);
                ++controls;

                if (! enSameFields (before, after) && ! eq::sameBandInput (before, after)
                      && eq::classifyBandChange (before, after) == eq::BandChange::structural)
                    ++controlsFired;
            }

            {
                raw.type = static_cast<float> (static_cast<int> (eq::Type::lowShelf));
                raw.slope = 1.0f;
                raw.q = 1.0f;
                eq::RawBand edited = raw;
                edited.q = 2.0f;
                const eq::BandInput before = eq::makeBandInput (raw, scale, 0.0);
                const eq::BandInput after = eq::makeBandInput (edited, scale, 0.0);
                ++controls;

                if (! enSameFields (before, after) && ! eq::sameBandInput (before, after)
                      && eq::classifyBandChange (before, after) == eq::BandChange::continuous)
                    ++controlsFired;
            }
        }

        row (slopeBad == 0 && qBad == 0 && slopeEdits > 0 && qEdits > 0 && controlsFired == controls,
             "SN3 canonical form: slope edits on Bell / Notch / Band Pass and Q edits at 6 dB/oct leave BandInput field-identical",
             strf ("%lld slope edits, %lld differ; %lld Q edits, %lld differ; controls (Low Cut slope edit -> structural, "
                   "12 dB/oct shelf Q edit -> continuous): %lld of %lld fire",
                   slopeEdits, slopeBad, qEdits, qBad, controlsFired, controls));
    }

    //==========================================================================
    // EN-dyn: the dynamic offset (DSP-07, engine half)
    //==========================================================================
    void rowEnDyn()
    {
        constexpr int numSamples = 1 << 15;
        constexpr int numDraws = 100000;

        eq::Designer designer;
        designer.prepare (48000.0);

        const eq::RawBand zeroBell = enRaw (eq::Type::bell, 1000.0, 0.0, 1.0, 1, eq::Placement::stereo);
        const eq::RawBand sixBell = enRaw (eq::Type::bell, 1000.0, 6.0, 1.0, 1, eq::Placement::stereo);

        // The hook is read, never written: Band::dynOffsetDb is 0 on a fresh band.
        const eq::Band fresh {};
        const bool hookZero = fresh.dynOffsetDb == 0.0;

        const eq::BandInput viaOffset = eq::makeBandInput (zeroBell, kEnScale100, 6.0);
        const eq::BandInput viaGain = eq::makeBandInput (sixBell, kEnScale100, 0.0);
        const eq::BandInput neutral = eq::makeBandInput (zeroBell, kEnScale100, fresh.dynOffsetDb);

        const bool gainSame = sameBits (viaOffset.settings.gainDbEff, viaGain.settings.gainDbEff)
                           && sameBits (viaGain.settings.gainDbEff, 6.0)
                           && enSameFields (viaOffset, viaGain);
        const bool neutralZero = neutral.settings.gainDbEff == 0.0;

        Rng rng (0x454E4459ull);
        std::vector<double> inL (ix (numSamples)), inR (ix (numSamples));
        enFillNoise (inL, rng);
        enFillNoise (inR, rng);

        const std::size_t bytes = sizeof (double) * ix (numSamples);
        std::vector<double> outL[3], outR[3];
        const eq::BandInput* const which[3] = { &viaOffset, &viaGain, &neutral };

        for (int k = 0; k < 3; ++k)
        {
            outL[k] = inL;
            outR[k] = inR;
            EnChain chain;
            chain.add (designer, *which[k]);
            chain.process (outL[k].data(), outR[k].data(), numSamples, true);
        }

        const bool renderSame = std::memcmp (outL[0].data(), outL[1].data(), bytes) == 0
                             && std::memcmp (outR[0].data(), outR[1].data(), bytes) == 0;
        const bool renderLive = std::memcmp (outL[0].data(), outL[2].data(), bytes) != 0
                             && std::memcmp (outL[0].data(), inL.data(), bytes) != 0;

        // Offset 0 (and -0) leaves gainDbEff at clamp (gain * scale / 100, -60, +60).
        long long unchangedBad = 0;
        eq::RawBand raw = zeroBell;

        for (int n = 0; n < numDraws; ++n)
        {
            raw.gain = static_cast<float> (60.0 * rng.uniform() - 30.0);
            const float scale = n % 4 == 0 ? kEnScale100 : static_cast<float> (200.0 * rng.uniform());

            const double product = static_cast<double> (raw.gain) * static_cast<double> (scale);
            const double scaled = product / 100.0;
            const double expected = scaled < -60.0 ? -60.0 : (scaled > 60.0 ? 60.0 : scaled);

            const double withZero = eq::makeBandInput (raw, scale, 0.0).settings.gainDbEff;
            const double withMinusZero = eq::makeBandInput (raw, scale, -0.0).settings.gainDbEff;
            const double withHook = eq::makeBandInput (raw, scale, fresh.dynOffsetDb).settings.gainDbEff;

            if (! (sameBits (withZero, expected) && sameBits (withMinusZero, expected) && sameBits (withHook, expected)))
                ++unchangedBad;
        }

        row (hookZero && gainSame && neutralZero && renderSame && renderLive && unchangedBad == 0,
             "EN-dyn offset +6 dB on a 0 dB bell == a +6 dB bell: gainDbEff and the rendered output bit for bit; offset 0 changes nothing",
             strf ("gainDbEff %.17g vs %.17g (%s); render %s (stereo noise, 48 kHz, %d samples)%s; Band::dynOffsetDb reads %g on a fresh band; "
                   "offset 0 / -0 / the hook's value over %d random gains and scales: %lld differ from clamp (gain * scale / 100)",
                   viaOffset.settings.gainDbEff, viaGain.settings.gainDbEff, gainSame ? "same bits" : "DIFFER",
                   renderSame ? "memcmp equal" : "DIFFERS", numSamples,
                   renderLive ? "" : " (VOID: the +6 dB render equals the 0 dB render or the input)",
                   fresh.dynOffsetDb, numDraws, unchangedBad));
    }

    //==========================================================================
    // --neg
    //==========================================================================
    /** Runs one negative control. Exit code 0 only if the targeted row FAILS. */
    int runNegative (const std::string& which)
    {
        if (which == "rbj")
        {
            std::vector<std::string> lines;
            double worstAt30 = 0.0;
            const bool pass = runG1 (G1Kind::bell, G1Source::rbj, G1Tolerance::phase21, { 44100.0 }, lines, worstAt30);

            for (const std::string& text : lines)
                note (text);

            std::printf ("neg rbj: G1-bell scorer on the RBJ build: %s (worst at +-30 dB %.2f dB)\n",
                         pass ? "PASS - the control did NOT fire" : "FAIL - the control fires", worstAt30);
            return pass ? 1 : 0;
        }

        if (which == "seed" || which == "rbj-b")
        {
            const bool seedControl = which == "seed";
            std::vector<std::string> lines;
            double worstAt30 = 0.0;
            const bool pass = runG1 (G1Kind::bell, seedControl ? G1Source::seed : G1Source::rbj, G1Tolerance::phase21b,
                                     { 44100.0 }, lines, worstAt30);

            for (const std::string& text : lines)
                note (text);

            std::printf ("neg %s: G1b-bell scorer on the %s: %s (worst at +-30 dB %.3f dB)\n", which.c_str(),
                         seedControl ? "closed-form seed" : "RBJ build",
                         pass ? "PASS - the control did NOT fire" : "FAIL - the control fires", worstAt30);
            return pass ? 1 : 0;
        }

        if (which == "binary-gate")
        {
            C1Summary sum;
            bool ranSecondStage = false;
            int excluded = 0;
            const bool fires = c1BinaryGateFires (nullptr, sum, ranSecondStage, excluded);
            c1Notes (sum);
            std::printf ("neg binary-gate: C1 on designWith<BinaryExtension>, lines the shipped design passes: %s (%s): %s\n",
                         c1Detail (sum).c_str(), c1BinaryGateScope (ranSecondStage, excluded).c_str(),
                         fires ? "C1 FAILS - the control fires" : "the control did NOT fire");
            return fires ? 0 : 1;
        }

        if (which == "first-build")
        {
            // NW2 and RA1 on the first build's draw.
            bool nw2Fires = false;
            long long runaway = 0;

            for (const double fs : { 44100.0, 48000.0 })
            {
                const Nw2Result r = runNw2<FirstBuildVariant> (fs);
                nw2Notes (fs, r);
                nw2Fires = nw2Fires || (! nw2Void (r) && ! nw2Passes (r));
                runaway += r.runaway;
            }

            const bool ra1Fires = runaway > 0;

            // C1, block A.
            const std::vector<C1Line> lines = c1Lines();
            const std::vector<C1Result> results = c1RunLines<FirstBuildVariant> (lines);
            C1Summary sum;
            c1Accumulate (lines, results, sum);
            c1Notes (sum);
            const bool c1Fires = sum.violations > 0;

            std::printf ("neg first-build: NW2 on the first build (no shelf re-centre, no step limit): %s\n",
                         nw2Fires ? "NW2 FAILS - the control fires" : "the control did NOT fire");
            std::printf ("neg first-build: RA1 on the first build: %lld runaway sections: %s\n", runaway,
                         ra1Fires ? "RA1 FAILS - the control fires" : "the control did NOT fire");
            std::printf ("neg first-build: C1 block A on the first build: %s: %s\n", c1Detail (sum).c_str(),
                         c1Fires ? "C1 FAILS - the control fires" : "the control did NOT fire");
            return (nw2Fires && ra1Fires && c1Fires) ? 0 : 1;
        }

        if (which == "f1")
        {
            std::string detail;
            const bool fires = f1NegativeFires (detail);
            std::printf ("neg f1: %s: %s\n", detail.c_str(), fires ? "the control fires" : "the control did NOT fire");
            return fires ? 0 : 1;
        }

        if (which == "alloc")
        {
            const AllocResult r = runA1 (true);
            const bool fires = r.available && r.allocs > 0;
            std::printf ("neg alloc: planted allocation inside the A1 window: %d allocations, %d frees counted: %s\n",
                         r.allocs, r.frees, fires ? "A1 FAILS - the control fires" : "the control did NOT fire");
            return fires ? 0 : 1;
        }

        if (which == "engine-alloc")
        {
            const AllocResult r = runEnAlloc (true);
            const bool fires = r.available && r.allocs > 0;
            std::printf ("neg engine-alloc: planted allocation inside the EN-alloc window: %d allocations, %d frees counted: %s\n",
                         r.allocs, r.frees, fires ? "EN-alloc FAILS - the control fires" : "the control did NOT fire");
            return fires ? 0 : 1;
        }

        std::fprintf (stderr, "unknown negative control '%s' (rbj, f1, alloc, seed, rbj-b, binary-gate, first-build, engine-alloc)\n",
                      which.c_str());
        return 255;
    }
}

//==============================================================================
int main (int argc, char** argv)
{
    std::string goldenDir = GOLDEN_DIR;
    std::string negative;
    bool dump = false, dumpPolished = false;

    for (int a = 1; a < argc; ++a)
    {
        const std::string arg = argv[a];

        if (arg == "--dump-designs")
        {
            dump = true;
        }
        else if (arg == "--dump-polished")
        {
            dumpPolished = true;
        }
        else if (arg == "--golden-dir" && a + 1 < argc)
        {
            goldenDir = argv[++a];
        }
        else if (arg == "--neg" && a + 1 < argc)
        {
            negative = argv[++a];
        }
        else
        {
            std::fprintf (stderr, "usage: %s [--golden-dir <dir>] [--dump-designs] [--dump-polished] "
                                  "[--neg rbj|f1|alloc|seed|rbj-b|binary-gate|first-build|engine-alloc]\n", argv[0]);
            return 255;
        }
    }

    if (dump)
        return dumpDesigns (false);

    if (dumpPolished)
        return dumpDesigns (true);

    if (! negative.empty())
        return runNegative (negative);

    std::printf ("O-ParametricEQ designer harness (Stage 2, Phases 2.1 + 2.1b)  golden dir: %s\n", goldenDir.c_str());
    std::printf ("Phase 2.1 rows score Designer::designSeed (the closed form); Phase 2.1b rows score Designer::design (polished).\n");

    heading ("D. golden and evaluator");
    rowD1 (goldenDir);
    rowE1();

    heading ("G1. magnitude match against the analog prototypes");
    rowsG1();
    rowT1();

    heading ("R / N / F. reciprocity, precision, robustness");
    rowReciprocity (false);
    rowN1();
    rowF1();
    {
        std::string detail;
        const bool fires = f1NegativeFires (detail);
        row (fires, "F1n the F1 checker FAILS a NaN and a -1", detail);
    }

    heading ("G2 / decisions D6, D8, D9, D10");
    rowG2s();
    rowSL1();
    rowQ1();
    rowTL1();
    rowZ1();
    rowP1();
    rowP1s();

    heading ("NB. notch and band pass below their clamps");
    std::vector<std::string> notchBandPassLines;
    rowsNB (notchBandPassLines);

    heading ("CS. cut skirts against the prototype (regression ceilings)");
    rowsCS();

    heading ("A / B. audio-thread fitness");
    rowAllocation (false);
    rowTwoThreads (false);

    heading ("INFO (never failing)");
    infoRows (notchBandPassLines);

    //==========================================================================
    // Phase 2.1b (PLAN-2.1b T6): Designer::design, the polished designer
    //==========================================================================
    heading ("2.1b / D2. the polish: constants and golden");
    printPolishConstants();
    rowD2 (goldenDir);

    heading ("2.1b / G1b. tightened magnitude match, and its negative controls");
    rowsG1b();

    heading ("2.1b / NW1, R2. never worse per section; reciprocity");
    rowNW1();
    rowReciprocity (true);

    heading ("2.1b / C1. continuity (1101 + 216 lines of 40 000 settings each, threaded)");
    const std::vector<bool> c1ShippedPass = rowC1();
    rowC1Hi();
    rowC1n (c1ShippedPass);

    heading ("2.1b / NW2, RA1. random full range: never far worse than the seed, no runaway fit (threaded)");
    rowsNW2RA1();

    heading ("2.1b / PR1, F2, S1, TL2. rate gate, robustness, skip exactness, tilt identity");
    rowPR1();
    rowF2();
    rowS1();
    rowTL2();

    heading ("2.1b / A2, B2. audio-thread fitness with the polish on");
    rowAllocation (true);
    rowTwoThreads (true);

    heading ("2.1b / INFO (never failing)");
    infoH1();
    infoOdd();
    infoWeak();
    infoNew();
    infoCost();

    //==========================================================================
    // Phase 2.2 (PLAN-2.2 T4): eq::Band + eq::Router in double
    //==========================================================================
    heading ("2.2 / engine (double)");
    rowsEnIr();
    rowsEnMs();
    rowEnLr();
    rowEnG8();
    rowEnBlk();
    rowEnFin();
    rowEnAlloc();
    rowEnSn1();
    rowEnSn2();
    rowEnSn3();
    rowEnDyn();

    std::printf ("\n%d PASS, %d FAIL, %d INFO\n", passes, failures, infos);
    std::fflush (stdout);
    return std::min (failures, 255);
}
