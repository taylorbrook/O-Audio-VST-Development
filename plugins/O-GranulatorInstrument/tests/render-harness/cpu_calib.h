#pragma once

// ============================================================================
// cpu_calib.h — machine-speed witness (PLAN-2.3 T2 / PD2, RESEARCH-2.3 P9).
//
// The duty witness (thread CPU / wall) cannot see a slowed core: a core held
// at a low clock runs the thread 100 % of the wall time and every CPU
// percentage simply grows. calibOnceNs() times a FIXED arithmetic workload —
// 5·10⁷ iterations of 4 independent double FMA chains (2·10⁸ FMAs) — on the
// thread clock; calibNs() is the median of 5. The reference ref_ns is frozen
// ONCE (T2, unmodified 2.2 engine) in tests/golden/cpu-calib-2.3.txt; every
// timed --cpu23 config is bracketed by calibNs() and judged against it.
//
// DO NOT EDIT after T2: the frozen reference is only meaningful for this
// exact workload (2.4 reads the same file).
// ============================================================================

#include <JuceHeader.h>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <limits>

namespace cal
{
    inline double threadNowNs() noexcept
    {
        timespec ts {};
        clock_gettime (CLOCK_THREAD_CPUTIME_ID, &ts);
        return (double) ts.tv_sec * 1.0e9 + (double) ts.tv_nsec;
    }

    inline volatile double gSink = 0.0;

    /** One timed pass of the fixed workload (thread CPU ns). */
    inline double calibOnceNs() noexcept
    {
        double x0 = 1.0, x1 = 2.0, x2 = 3.0, x3 = 4.0;
        const double t0 = threadNowNs();
        for (int i = 0; i < 50000000; ++i)
        {
            x0 = std::fma (x0, 0.999999999, 1.0e-9);
            x1 = std::fma (x1, 0.999999999, 1.0e-9);
            x2 = std::fma (x2, 0.999999999, 1.0e-9);
            x3 = std::fma (x3, 0.999999999, 1.0e-9);
        }
        const double t1 = threadNowNs();
        gSink = x0 + x1 + x2 + x3;
        return t1 - t0;
    }

    /** Median of 5 passes (ns). */
    inline double calibNs() noexcept
    {
        double v[5];
        for (auto& x : v)
            x = calibOnceNs();
        std::sort (v, v + 5);
        return v[2];
    }

    /** ref_ns from <goldenDir>/cpu-calib-2.3.txt ("ref_ns <value>" line), or NaN. */
    inline double readRef (const juce::File& goldenDir)
    {
        const auto f = goldenDir.getChildFile ("cpu-calib-2.3.txt");
        if (! f.existsAsFile())
            return std::numeric_limits<double>::quiet_NaN();
        juce::StringArray lines;
        f.readLines (lines);
        for (const auto& l : lines)
            if (l.trim().startsWith ("ref_ns"))
                return l.trim().fromFirstOccurrenceOf ("ref_ns", false, false).trim().getDoubleValue();
        return std::numeric_limits<double>::quiet_NaN();
    }

    /** --calib: prints "calib median <ns> samples <5 values>". */
    inline int runCalib()
    {
        double v[5];
        for (auto& x : v)
            x = calibOnceNs();
        double s[5];
        std::copy (v, v + 5, s);
        std::sort (s, s + 5);
        std::printf ("calib median %.0f samples %.0f %.0f %.0f %.0f %.0f\n", s[2], v[0], v[1], v[2], v[3], v[4]);
        std::fflush (stdout);
        return 0;
    }
} // namespace cal
