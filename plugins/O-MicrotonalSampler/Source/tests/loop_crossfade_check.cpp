/*
  ==============================================================================

    loop_crossfade_check.cpp
    O-MicrotonalSampler — v1.23.2 voice-DSP RT-safety + loop-crossfade regression.

    Manual run:
        ninja O-MicrotonalSampler_LoopCrossfadeCheck \
          && ./build/plugins/O-MicrotonalSampler/O-MicrotonalSampler_LoopCrossfadeCheck
    Exit code = number of failed assertions (0 = all pass).

    Why this test exists
    --------------------
    The 2026-06-30 DSP/voice review (REVIEW-dsp-voice.md) flagged three defects in
    the per-sample varispeed read path. This test drives the REAL helpers now
    living in VoiceDsp.h (extracted from MicrotonalSamplerVoice.cpp for exactly
    this purpose — not a mirror copy), so a future edit that regresses them fails
    here instead of shipping silently:

      W9  (WR-01): the 8-sample loop crossfade left a residual ~0.195×tail step
                   at every wrap (per-cycle click) because its LUT weight never
                   reached 1.0. Fixed to a continuous phase reaching full incoming
                   weight at the wrap.
      W11 (WR-03): computePlayRateForVariant divided by hostSR with no guard
                   (hostSR==0 → +Inf playRate) and wrapLoopPosition could spin
                   forever on a non-finite pos. Fixed to clamp the divisor and
                   snap a non-finite pos back to the loop start.

    Also pins the equalPowerWeights end-point contract that W9 depends on.

  ==============================================================================
*/

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include "../VoiceDsp.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

using namespace OMtsVoiceDsp;

namespace
{
    int failed = 0;

    void check (bool cond, const std::string& desc)
    {
        if (cond)
            std::cout << "  PASS: " << desc << "\n";
        else
        {
            std::cout << "  FAIL: " << desc << "\n";
            ++failed;
        }
    }

    bool approx (double a, double b, double eps) { return std::abs (a - b) <= eps; }
}

int main()
{
    std::cout << "== loop_crossfade_check ==\n";

    // ----------------------------------------------------------------
    // 1. equalPowerWeights end-points + power sum. W9 relies on the
    //    incoming weight actually REACHING 1.0 at x == 1 (the old LUT
    //    capped at equalPowerWeights(7/8) → incoming ≈ 0.981).
    // ----------------------------------------------------------------
    {
        const auto w0 = equalPowerWeights (0.0f);
        check (approx (w0.first, 1.0, 1e-5) && approx (w0.second, 0.0, 1e-5),
               "equalPowerWeights(0) = (1, 0)");

        const auto w1 = equalPowerWeights (1.0f);
        check (approx (w1.first, 0.0, 1e-5) && approx (w1.second, 1.0, 1e-5),
               "equalPowerWeights(1) = (0, 1) — incoming reaches full weight");

        const auto wh = equalPowerWeights (0.5f);
        check (approx (wh.first, wh.second, 1e-5),
               "equalPowerWeights(0.5) is symmetric");

        bool powerFlat = true;
        for (int i = 0; i <= 20; ++i)
        {
            const auto w = equalPowerWeights ((float) i / 20.0f);
            const double p = (double) w.first * w.first + (double) w.second * w.second;
            if (! approx (p, 1.0, 1e-4)) powerFlat = false;
        }
        check (powerFlat, "equalPowerWeights keeps cos²+sin² = 1 across the sweep");

        // jlimit guard: out-of-range x clamps rather than overshoots.
        const auto wOver = equalPowerWeights (2.0f);
        const auto wNeg  = equalPowerWeights (-1.0f);
        check (approx (wOver.first, 0.0, 1e-5) && approx (wNeg.first, 1.0, 1e-5),
               "equalPowerWeights clamps x to [0,1]");
    }

    // ----------------------------------------------------------------
    // 2. W9 — loop crossfade reaches full INCOMING weight at the wrap
    //    (no residual tail step = no per-cycle click).
    //
    //    Buffer with a +1.0 OUTGOING plateau around the loop tail and a
    //    -1.0 INCOMING plateau around the loop head + the post-wrap read
    //    region. A correct crossfade hands off from +1.0 at fadeStart to
    //    -1.0 at the wrap; the old 7/8-capped LUT stopped at
    //    0.195·(+1) + 0.981·(-1) ≈ -0.786, so it stepped 0.214 to the
    //    post-wrap -1.0 signal every cycle.
    // ----------------------------------------------------------------
    {
        const int N       = 220;
        const int lpStart = 100;
        const int lpEnd   = 160;              // lpLen = 60, fadeStart = 152

        std::vector<float> buf ((size_t) N, 0.0f);
        for (int i = 148; i <= 164; ++i) buf[(size_t) i] =  1.0f;   // outgoing (tail)
        for (int i = 96;  i <= 112; ++i) buf[(size_t) i] = -1.0f;   // incoming (head) + post-wrap

        const float* b = buf.data();

        // Fade START (x=0): full outgoing weight → reads the +1.0 tail.
        const float yStart = readVariantWithLoop (b, N, (double) (lpEnd - 8), lpStart, lpEnd);
        check (approx (yStart, 1.0, 0.02),
               "W9: crossfade starts at full outgoing (+1.0) at fadeStart");

        // Just BEFORE the wrap (x → 1): full incoming weight → reads the -1.0 head.
        // This is the assertion the old capped LUT failed (it returned ≈ -0.786).
        const float yBeforeWrap = readVariantWithLoop (b, N, (double) lpEnd - 1e-4, lpStart, lpEnd);
        check (approx (yBeforeWrap, -1.0, 0.02),
               "W9: crossfade reaches full incoming (-1.0) at the wrap (no tail step)");

        // The click is the jump from the last pre-wrap sample to the first
        // post-wrap sample. After wrap, pos ≈ lpStart reads the -1.0 head
        // directly (pos < fadeStart → plain cubic). Continuous ⇒ ~0 click.
        const float yAfterWrap = readVariantWithLoop (b, N, (double) lpStart, lpStart, lpEnd);
        const float clickMag   = std::abs (yBeforeWrap - yAfterWrap);
        check (clickMag < 0.02,
               "W9: wrap discontinuity (click) is < 0.02 (was ≈ 0.214 pre-fix)");

        // pos below the fade region (inside the head plateau) is an untouched
        // cubic read (no crossfade blending).
        const float yMid = readVariantWithLoop (b, N, 104.0, lpStart, lpEnd);
        check (approx (yMid, -1.0, 1e-4),
               "W9: pos before fadeStart is a plain cubic read (no crossfade)");
    }

    // ----------------------------------------------------------------
    // 3. W11 — wrapLoopPosition is finite-safe. A non-finite pos must not
    //    spin the `while (pos >= lpEnd)` loop forever; snap to loop start.
    // ----------------------------------------------------------------
    {
        const int lpStart = 100;
        const int lpEnd   = 160;

        double posInf = std::numeric_limits<double>::infinity();
        wrapLoopPosition (posInf, lpStart, lpEnd);
        check (std::isfinite (posInf) && approx (posInf, lpStart, 1e-9),
               "W11: +Inf pos snaps to loop start (finite)");

        double posNegInf = -std::numeric_limits<double>::infinity();
        wrapLoopPosition (posNegInf, lpStart, lpEnd);
        check (std::isfinite (posNegInf) && approx (posNegInf, lpStart, 1e-9),
               "W11: -Inf pos snaps to loop start (finite)");

        double posNan = std::numeric_limits<double>::quiet_NaN();
        wrapLoopPosition (posNan, lpStart, lpEnd);
        check (std::isfinite (posNan) && approx (posNan, lpStart, 1e-9),
               "W11: NaN pos snaps to loop start (finite)");

        // Normal wrapping still works.
        double p1 = 200.0; wrapLoopPosition (p1, lpStart, lpEnd);
        check (approx (p1, 140.0, 1e-9), "W11: pos=200 wraps to 140 (in [100,160))");

        double p2 = 160.0; wrapLoopPosition (p2, lpStart, lpEnd);
        check (approx (p2, 100.0, 1e-9), "W11: pos=lpEnd wraps to loop start");

        double p3 = 150.0; wrapLoopPosition (p3, lpStart, lpEnd);
        check (approx (p3, 150.0, 1e-9), "W11: pos inside loop is unchanged");

        // Degenerate loops are left untouched (one-shot / zero-length).
        double p4 = 999.0; wrapLoopPosition (p4, 0, 0);
        check (approx (p4, 999.0, 1e-9), "W11: lpEnd<=0 (one-shot) leaves pos untouched");

        double p5 = 999.0; wrapLoopPosition (p5, 160, 160);
        check (approx (p5, 999.0, 1e-9), "W11: zero-length loop leaves pos untouched");
    }

    // ----------------------------------------------------------------
    // 4. W11 — computePlayRateForVariant guards the hostSR divisor. A zero
    //    host sample rate must not yield +Inf (which fed the wrap hang above).
    // ----------------------------------------------------------------
    {
        SampleVariant v;
        v.sourceSampleRate = 48000.0;
        const int    midi  = 69;      // A440 reference
        const double freq  = 440.0;

        const double rNormal = computePlayRateForVariant (v, midi, freq, 48000.0);
        check (approx (rNormal, 1.0, 1e-9),
               "W11: 48k source @ 48k host, A440 → playRate 1.0");

        const double rZero = computePlayRateForVariant (v, midi, freq, 0.0);
        check (std::isfinite (rZero), "W11: hostSR=0 yields a FINITE playRate (not +Inf)");

        const double r441 = computePlayRateForVariant (v, midi, freq, 44100.0);
        check (approx (rZero, r441, 1e-9),
               "W11: hostSR=0 clamps to 44100 (matches explicit 44100 result)");

        // Both source and host zero → still finite (slotSR falls back to the
        // clamped divisor → unity).
        SampleVariant vz; vz.sourceSampleRate = 0.0;
        const double rBothZero = computePlayRateForVariant (vz, midi, freq, 0.0);
        check (std::isfinite (rBothZero) && approx (rBothZero, 1.0, 1e-9),
               "W11: source=0 AND host=0 → finite unity rate");
    }

    std::cout << "== loop_crossfade_check: "
              << (failed == 0 ? "ALL PASS" : "FAIL")
              << " (" << failed << " failures) ==\n";
    return failed;
}
