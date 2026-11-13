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

    O-ParametricEQ - designer harness (Stage 2, Phase 2.1; offline gate G1)

      O-ParametricEQ-designer-harness                      run every row
      O-ParametricEQ-designer-harness --golden-dir <dir>   compare D1 against <dir>/designs.tsv
      O-ParametricEQ-designer-harness --dump-designs       print the golden's settings list as
                                                           designed by the C++, exit 0
      O-ParametricEQ-designer-harness --neg rbj|f1|alloc   run ONE negative control; exit 0 only
                                                           if the targeted row FAILS

    Exit code = number of FAIL rows (capped at 255). Last line: "N PASS, M FAIL, K INFO".

    Plain C++17: the four Source/dsp headers and the standard library. The
    golden (tests/golden/designs.tsv) is written by tests/model/dump_designs.py,
    never by this program: --dump-designs goes to stdout for comparison only.

      D1       C++ design of every golden line vs designs.tsv (1e-9 relative)
      E1       ResponseEval vs the SVF / one-pole recurrences run on an impulse
      G1-bell  G1, bells, five rates
      G1-sh12  G1, 12 dB/oct shelves, Q <= 1.0, five rates
      G1-sh6   G1, 6 dB/oct shelves, five rates
      G1n      an RBJ bilinear bell scored by the G1-bell code must FAIL
      T1       the eleven "Model results" rows within 0.05 dB
      R1       boost / cut reciprocity within 1e-9 dB
      N1       10 Hz, Q 40, +30 dB bell at 192 kHz within 1e-6 dB
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

#include <algorithm>
#include <array>
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

    eq::BandDesign designAt (double fs, const eq::BandSettings& s)
    {
        eq::Designer designer;
        designer.prepare (fs);
        eq::BandDesign d {};
        designer.design (s, d);
        return d;
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

    /** Worst maxError over types x gains x fcs x qs at one rate. */
    double zoneWorst (double fs, std::initializer_list<eq::Type> types, int slopeIndex,
                      const std::vector<double>& fcs, const std::vector<double>& qs,
                      std::initializer_list<double> gains, const Grid& grid, bool withTied,
                      double floorDb = -1.0e9)
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
                        designer.design (mk (type, fc, gain, q, slopeIndex), d);
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

    void appendNumber (std::string& out, double x)
    {
        if (! std::isfinite (x))
        {
            std::fprintf (stderr, "designer-harness: non-finite value in --dump-designs\n");
            std::exit (2);
        }

        char text[48];
        std::snprintf (text, sizeof (text), "%.17g", x + 0.0);      // + 0.0 turns a negative zero into 0
        out += text;
    }

    /** stdout only, "\n" endings only, the structure of dump_designs.py. */
    int dumpDesigns()
    {
        std::fputs ("# GENERATED by tests/model/dump_designs.py; do not edit\n", stdout);
        std::fputs ("# fs\ttype\tslope\tfc\tq\tgain\tsec\tcls\thasSvf\thasOnePole\tg\tk\tmL\tmB\tmH\tG\tcL\tcH\n", stdout);

        eq::Designer designer;
        eq::BandDesign d {};
        std::string head, line;

        for (const GoldenSetting& gs : goldenSettings())
        {
            designer.prepare (gs.fs);
            designer.design (mk (static_cast<eq::Type> (gs.type), gs.fc, gs.gain, gs.q, gs.slope), d);

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
                designer.design (mk (static_cast<eq::Type> (next.type), next.fc, next.gain, next.q, next.slope), current);
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
    enum class G1Kind { bell, shelf12, shelf6 };

    double g1Tolerance (G1Kind kind, double fs, double gainAbs)
    {
        if (fs >= 88200.0)
            return 0.10 + 0.015 * gainAbs;

        switch (kind)
        {
            case G1Kind::bell:      return 0.15 + 0.05 * gainAbs;
            case G1Kind::shelf12:   return 0.15 + 0.06 * gainAbs;
            case G1Kind::shelf6:    return 0.10 + 0.02 * gainAbs;       // P9: not tightened
        }

        return 0.0;
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
        line per rate (worst / tolerance per gain) and the worst +-30 dB error. */
    bool runG1 (G1Kind kind, bool rbj, std::initializer_list<double> rates,
                std::vector<std::string>& lines, double& worstAt30)
    {
        const std::vector<double> fcs = geomspace (20.0, 20000.0, 40);
        std::vector<double> qs;

        if (kind == G1Kind::bell)
        {
            qs = geomspace (0.025, 40.0, 17);
        }
        else if (kind == G1Kind::shelf12)
        {
            for (const double q : geomspace (0.025, 40.0, 17))
                if (q <= 1.0 + 1.0e-9)
                    qs.push_back (q);

            qs.push_back (0.70710678);
            qs.push_back (1.0);
        }
        else
        {
            qs.push_back (1.0);     // not read at 6 dB/oct
        }

        const int slope = kind == G1Kind::shelf6 ? 0 : 1;
        bool pass = true;
        worstAt30 = 0.0;

        for (const double fs : rates)
        {
            const Grid grid = makeGrid (fs, 1000, 20.0, 20000.0, 0.499);
            eq::Designer designer;
            designer.prepare (fs);
            eq::BandDesign analog {}, digital {};
            std::string line = strf ("fs %6.0f  worst/tol:", fs);

            for (const double gainAbs : kG1Gains)
            {
                double worst = 0.0;

                for (const double sign : { 1.0, -1.0 })
                    for (const double fc : fcs)
                        for (const double q : qs)
                        {
                            if (kind == G1Kind::bell)
                            {
                                designer.design (mk (eq::Type::bell, fc, sign * gainAbs, q, slope), analog);
                                digital = analog;

                                if (rbj)
                                    digital.params[0] = rbjBell (fs, fc, sign * gainAbs, q);

                                worst = std::max (worst, maxError (digital, analog, grid, true));
                            }
                            else
                            {
                                for (const eq::Type type : { eq::Type::lowShelf, eq::Type::highShelf })
                                {
                                    designer.design (mk (type, fc, sign * gainAbs, q, slope), analog);
                                    worst = std::max (worst, maxError (analog, analog, grid, true));
                                }
                            }
                        }

                const double tol = g1Tolerance (kind, fs, gainAbs);

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
            const bool pass = runG1 (family.kind, false, { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 }, lines, worstAt30);
            row (pass, family.name, "five rates, fc 20 Hz-20 kHz x 40, +-3/6/12/18/30 dB");

            for (const std::string& text : lines)
                note (text);
        }

        // G1n: the control must FAIL the same scorer.
        std::vector<std::string> lines;
        double worstAt30 = 0.0;
        const bool rbjPass = runG1 (G1Kind::bell, true, { 44100.0 }, lines, worstAt30);
        row (! rbjPass, "G1n RBJ bilinear bell FAILS the G1-bell tolerance at 44.1 kHz",
             strf ("worst at +-30 dB %.2f dB (model 13-17)", worstAt30));

        for (const std::string& text : lines)
            note (text);
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
            { "shelf48 Q1 10-20k",           44100.0, true,  5, 10000.0, 20000.0, 1, { 0.84, 1.72, 4.01 } },
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
                             const std::vector<double>& qs, const Grid& grid, bool& flagsOk)
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
                    designer.design (mk (type, fc, gainAbs, q, slopeIndex), boost);
                    designer.design (mk (type, fc, -gainAbs, q, slopeIndex), cut);
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

    void rowR1()
    {
        const std::vector<double> fcs = geomspace (20.0, 20000.0, 40);
        const std::vector<double> bellQs = geomspace (0.025, 40.0, 17);
        const std::vector<double> shelfQs { 0.3, 1.0, 4.0 };
        double worstBell = 0.0, worstShelf = 0.0;
        bool flagsOk = true;

        for (const double fs : kRates5)
        {
            const Grid grid = makeGrid (fs, 1000, 20.0, 20000.0, 0.499);
            worstBell = std::max (worstBell, reciprocityWorst (fs, eq::Type::bell, 1, fcs, bellQs, grid, flagsOk));

            for (const eq::Type type : { eq::Type::lowShelf, eq::Type::highShelf, eq::Type::tilt })
                for (int si = 0; si < 8; ++si)
                    worstShelf = std::max (worstShelf, reciprocityWorst (fs, type, si, fcs, shelfQs, grid, flagsOk));
        }

        row (flagsOk && worstBell <= 1.0e-9 && worstShelf <= 1.0e-9,
             "R1 |H(+g) H(-g)| = 1 within 1e-9 dB (bells; shelves and tilt, 8 slopes)",
             strf ("bells %.3g dB, shelves / tilt %.3g dB, five rates%s", worstBell, worstShelf,
                   flagsOk ? "" : "; inverted flag WRONG"));
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

        row (worst <= 1.0e-6, "N1 10 Hz, Q 40, +30 dB bell at 192 kHz vs prototype, 2-50 Hz",
             strf ("%.3g dB (limit 1e-6)", worst));

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
                designers[r].design (s, d);
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

        for (const eq::Type type : { eq::Type::lowCut, eq::Type::highCut })
            for (int si = 0; si < 8; ++si)
            {
                const double nominal = 20.0 * std::log10 (2.0) * static_cast<double> (eq::slopeOrder[si]);

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
                            ++slopeBad;             // a low cut always has both points
                        else
                            ++highCutSkipped;

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
        A separate short window with a deliberate allocation proves the hook live. */
    AllocResult runA1 (bool plant)
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
            designers[ix (i & 3)].design (settings[ix (i)], d);
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
       #endif

        return result;
    }

    void rowA1()
    {
        const AllocResult r = runA1 (false);

        if (! r.available)
        {
            info ("A1 allocation gate SKIPPED: no malloc_logger on this platform");
            return;
        }

        const bool hookLive = r.vacuityAllocs >= 1 && r.vacuityFrees >= 1;
        row (hookLive && r.allocs == 0 && r.frees == 0, "A1 1e5 design() calls: 0 allocations, 0 frees (malloc_logger, this thread)",
             strf ("%d allocations, %d frees; hook %s (planted: %d / %d); %lld sections designed",
                   r.allocs, r.frees, hookLive ? "live" : "DEAD", r.vacuityAllocs, r.vacuityFrees, r.checksum));
    }

    //==========================================================================
    // B1: two threads
    //==========================================================================
    void rowB1()
    {
        constexpr int numSettings = 10000;
        std::vector<eq::BandSettings> settings;
        Rng rng (0xB1B1B1B1ull);

        for (int i = 0; i < numSettings; ++i)
            settings.push_back (randomSettings (rng));

        std::vector<eq::BandDesign> onMain (ix (numSettings)), onSecond (ix (numSettings));

        const auto work = [&settings] (std::vector<eq::BandDesign>& out)
        {
            std::array<eq::Designer, 5> designers;      // this thread's own

            for (std::size_t r = 0; r < kRates5.size(); ++r)
                designers[r].prepare (kRates5[r]);

            for (std::size_t i = 0; i < settings.size(); ++i)
                designers[i % kRates5.size()].design (settings[i], out[i]);
        };

        work (onMain);
        std::thread second (work, std::ref (onSecond));
        second.join();

        int bad = 0;

        for (int i = 0; i < numSettings; ++i)
            if (! sameDesign (onMain[ix (i)], onSecond[ix (i)], true))
                ++bad;

        row (bad == 0, "B1 10 000 mixed settings: main thread == second thread with its own Designer (memcmp)",
             strf ("%d/%d", numSettings - bad, numSettings));
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

        info ("shelves >= 24 dB/oct, Q 1, fc 10-20 k at 44.1 kHz, +-6 / +-12 / +-30 dB (model 0.63/1.24/2.31, 0.84/1.72/4.01, 0.87/1.77/4.91)");
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
    // --neg
    //==========================================================================
    /** Runs one negative control. Exit code 0 only if the targeted row FAILS. */
    int runNegative (const std::string& which)
    {
        if (which == "rbj")
        {
            std::vector<std::string> lines;
            double worstAt30 = 0.0;
            const bool pass = runG1 (G1Kind::bell, true, { 44100.0 }, lines, worstAt30);

            for (const std::string& text : lines)
                note (text);

            std::printf ("neg rbj: G1-bell scorer on the RBJ build: %s (worst at +-30 dB %.2f dB)\n",
                         pass ? "PASS - the control did NOT fire" : "FAIL - the control fires", worstAt30);
            return pass ? 1 : 0;
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

        std::fprintf (stderr, "unknown negative control '%s' (rbj, f1, alloc)\n", which.c_str());
        return 255;
    }
}

//==============================================================================
int main (int argc, char** argv)
{
    std::string goldenDir = GOLDEN_DIR;
    std::string negative;
    bool dump = false;

    for (int a = 1; a < argc; ++a)
    {
        const std::string arg = argv[a];

        if (arg == "--dump-designs")
        {
            dump = true;
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
            std::fprintf (stderr, "usage: %s [--golden-dir <dir>] [--dump-designs] [--neg rbj|f1|alloc]\n", argv[0]);
            return 255;
        }
    }

    if (dump)
        return dumpDesigns();

    if (! negative.empty())
        return runNegative (negative);

    std::printf ("O-ParametricEQ designer harness (Stage 2, Phase 2.1)  golden dir: %s\n", goldenDir.c_str());

    heading ("D. golden and evaluator");
    rowD1 (goldenDir);
    rowE1();

    heading ("G1. magnitude match against the analog prototypes");
    rowsG1();
    rowT1();

    heading ("R / N / F. reciprocity, precision, robustness");
    rowR1();
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

    heading ("NB. notch and band pass below their clamps");
    std::vector<std::string> notchBandPassLines;
    rowsNB (notchBandPassLines);

    heading ("A / B. audio-thread fitness");
    rowA1();
    rowB1();

    heading ("INFO (never failing)");
    infoRows (notchBandPassLines);

    std::printf ("\n%d PASS, %d FAIL, %d INFO\n", passes, failures, infos);
    std::fflush (stdout);
    return std::min (failures, 255);
}
