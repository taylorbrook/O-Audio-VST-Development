/*
  ==============================================================================

    O-simpleBeatmaker - TimingFeelEngine (swing + humanize + quantize -> Δt) — THE LESSON

    Composes the per-hit sample offset and velocity. The terms are kept SEPARATE
    (never folded) so quantize touches ONLY humanize:

        dSwing       = (k odd) ? (swing01/3)*T8 : 0          // deterministic, NOT scaled by q
        dHumanT      = triRand01() * humanize01 * 30 ms       // Fallback A: LATE-ONLY [0,+30ms]
        dHumanV      = triangular() * humanize01 * 24          // symmetric +-24
        dtSec        = dSwing + dHumanT*(1-q)
        finalVel     = clamp(stepVel + dHumanV*(1-q), 1, 127)

    DSP-04 invariant (the entire quantize-vs-feel lesson): at q=1 the humanize
    terms vanish but dSwing SURVIVES.

    Fallback A (blessed by ARCHITECTURE Risk §Fallback A): timing humanize is
    one-sided positive ("laid back"), which removes negative offsets and the
    cross-block lookahead from the critical path. Velocity humanize stays
    symmetric. Symmetric (early) timing humanize is a documented post-gate
    enhancement.

    RNG is pre-seeded per voice in prepareToPlay and sampled once per committed
    hit — never reseeded on the audio thread (PERF-01).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <cmath>
#include "BeatmakerIDs.h"

namespace OSimpleBeatmaker
{
    struct AppliedHit
    {
        int         appliedOffsetSamples = 0;
        int         finalVelocity        = 0;
        juce::int32 nominalSampleInBar   = 0;
        juce::int32 appliedSampleInBar   = 0;
    };

    class TimingFeelEngine
    {
    public:
        static constexpr double kHumanizeMaxSec = 0.030;  // +30 ms (late-only)
        static constexpr double kHumanizeMaxVel = 24.0;   // +-24

        void prepareToPlay (double fsIn) noexcept
        {
            fs = fsIn;
            // Distinct, deterministic per-voice seeds. Seeded ONCE; never on the
            // audio thread.
            for (int v = 0; v < kNumVoices; ++v)
                rng[v].setSeed ((juce::int64) (0x9E3779B97F4A7C15ULL + (juce::uint64) v * 0x632BE59BD9B4E019ULL));
        }

        // Compute Δt + velocity for one committed hit. `stepIndex` selects the
        // swing parity; `nominalSampleInBar` is bar-relative (from the clock).
        AppliedHit compute (int voiceIndex, int stepIndex, juce::int32 nominalSampleInBar,
                            int stepVelocity, double bpm,
                            double swing01, double q, double humanize01) noexcept
        {
            if (bpm <= 1.0) bpm = 120.0;
            const int v = juce::jlimit (0, kNumVoices - 1, voiceIndex);

            const double T8     = (60.0 / bpm) / 2.0;                       // one 8th in seconds
            const double dSwing = ((stepIndex % 2) == 1) ? (swing01 / 3.0) * T8 : 0.0;

            const double dHumanT = triRand01 (rng[v]) * humanize01 * kHumanizeMaxSec;  // [0,+30ms]
            const double dHumanV = triangular (rng[v]) * humanize01 * kHumanizeMaxVel; // +-24

            const double dtSec   = dSwing + dHumanT * (1.0 - q);
            const int    applied = (int) std::llround (dtSec * fs);
            const int    finalV  = juce::jlimit (1, 127,
                                       (int) std::lround ((double) stepVelocity + dHumanV * (1.0 - q)));

            AppliedHit h;
            h.appliedOffsetSamples = applied;
            h.finalVelocity        = finalV;
            h.nominalSampleInBar   = nominalSampleInBar;
            h.appliedSampleInBar   = nominalSampleInBar + applied;
            return h;
        }

    private:
        // Sum-of-two-uniforms, centre-weighted [-1, 1].
        static double triangular (juce::Random& r) noexcept
        {
            return ((double) r.nextFloat() * 2.0 - 1.0 + (double) r.nextFloat() * 2.0 - 1.0) * 0.5;
        }
        // Centre-weighted [0, 1] (late-only humanize).
        static double triRand01 (juce::Random& r) noexcept
        {
            return ((double) r.nextFloat() + (double) r.nextFloat()) * 0.5;
        }

        double fs = 44100.0;
        juce::Random rng[kNumVoices];
    };
}
