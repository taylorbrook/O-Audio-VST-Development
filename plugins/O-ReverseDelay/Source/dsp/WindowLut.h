/*
  ==============================================================================

    O-ReverseDelay - WindowLut

    Precomputed grain-envelope tables, built once at construction and read with
    linear interpolation by grain phase 0..1. No per-sample transcendental in
    the grain loop.

    v1.0.0 shipped a single hard-coded Hann table, trimmed from O-simpleGrain's
    5-shape WindowLuts.h. v1.2.0 (B1) restores the family — five shapes plus a
    continuous TILT that moves the window's midpoint within the grain — because
    this is a REVERSE delay: every grain plays its source backwards, and under a
    symmetric Hann each one swells in and out identically, so the effect smears
    but never blooms. A window whose peak sits late produces the swell-into-a-
    transient shape the effect is bought for.

    Shape index order matches the grainShape AudioParameterChoice in
    PluginProcessor.cpp. Hann is index 0 so the default is the shipped sound:

        0 = Hann        0.5(1 - cos 2πφ)                     — v1.0/v1.1 default
        1 = Tukey       50 % cosine taper, flat middle       — the "open" one
        2 = Gaussian    σ = 0.18, PEDESTAL REMOVED (below)
        3 = Triangular  1 - |2φ - 1|
        4 = Expo-Decay  smooth 2 % attack, then e^-5u to zero — "plucked"

    ── The Gaussian pedestal (deliberate deviation from O-simpleGrain) ────────
    O-simpleGrain keeps the raw Gaussian, which at σ = 0.18 reads 0.021 at both
    ends rather than 0. In a one-shot granular texture that 2 % step is
    inaudible; here it is a step at EVERY grain boundary, overlapping 2-8 deep,
    inside a feedback loop that re-reverses it every generation. The pedestal is
    therefore subtracted and the result renormalised so w(0) = w(1) = 0 exactly
    and w(0.5) = 1 exactly.

    ── Tilt: a two-segment linear phase warp ─────────────────────────────────
    A warped lookup read(pow(phase, k)) would put a transcendental back in the
    grain loop, and a family of pre-tilted LUTs would cost ~2.6 MB for a
    quantised approximation of a continuous control. Instead the phase is warped
    by two linear segments that map [0, t] -> [0, 0.5] and [t, 1] -> [0.5, 1]:

        q = min(p, t)·(0.5/t) + max(p - t, 0)·(0.5/(1 - t))

    Two properties make this the right shape, and both are exact rather than
    approximate:

      1. AT t = 0.5 IT IS THE BITWISE IDENTITY. a = b = 1.0f exactly, so
         q = min(p, 0.5) + max(p - 0.5, 0). For p >= 0.5, Sterbenz's lemma makes
         p - 0.5 exact, and 0.5 + (p - 0.5) rounds to exactly p. The default
         tilt therefore reproduces the v1.1.0 render bit-for-bit — render-harness
         probe Z1 asserts that rather than trusting it.

      2. IT IS POWER-INVARIANT FOR SYMMETRIC WINDOWS. The two segments have
         Jacobians 2t and 2(1-t), so the warped mean square is
         t·mLo + (1-t)·mHi, which for mLo == mHi is just m — independent of t.
         Tilt cannot move the wet level or the feedback loop's duty cycle for
         Hann/Tukey/Gaussian/Triangular; getTiltNorm() returns exactly 1.0f for
         those and only does real work for the asymmetric Expo-Decay.

    Because the warp keys on the window's MIDPOINT, tilt moves the literal peak
    for the four symmetric shapes; for Expo-Decay (whose peak already sits at
    φ ≈ 0.02) it stretches the early decay against the late one instead.

    ── Power normalisation (the load-bearing part) ───────────────────────────
    PluginProcessor's grainGain = 1/sqrt(overlap) assumes HANN's power duty
    cycle, and the v1.0.0 CHANGELOG records the feedback loop's -4.3 dB per
    generation as specifically the Hann² duty. Tukey's mean square is 0.6875
    against Hann's 0.375, so swapping shapes at a fixed density would jump the
    wet level by 2.6 dB AND change the decay rate — the control would read as a
    volume and a feedback knob rather than as a timbre. getShapeNorm() folds
    sqrt(m_hann / m_shape) into grainGain so it reads as timbre.

    Every table and every constant here is built in the CONSTRUCTOR. Nothing in
    this file may be called from prepareToPlay or processBlock except the
    read/normalisation accessors, which only read.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <vector>

class WindowLut
{
public:
    //==========================================================================
    // Shape order IS the grainShape parameter's choice order. Hann must stay at
    // index 0: a choice parameter's default is an index, and index 0 is what an
    // absent key in a v1.0/v1.1 preset resolves to.
    enum Shape
    {
        hann = 0,
        tukey,
        gaussian,
        triangular,
        expoDecay
    };

    static constexpr int kNumShapes = 5;

    /** Peak-position bounds for the tilt warp. Not 0 and 1: both would divide by
        zero, and even approaching them collapses one half of the window into a
        handful of samples. ±0.45 about the centre keeps the shortest half at 5 %
        of the grain — 2.5 ms at the 50 ms grainSize minimum, a fast edge but not
        a step. */
    static constexpr float kMinPeakPos = 0.05f;
    static constexpr float kMaxPeakPos = 0.95f;

    explicit WindowLut (int lutSize = 2048)
        : size (juce::jmax (2, lutSize))
    {
        build();
    }

    //==========================================================================
    // Hot path. The caller resolves the table pointer ONCE per grain per render
    // pass (out of the per-sample loop) and reads through it, so the shape index
    // never costs a bounds check inside the loop.

    const float* getTable (int shape) const noexcept
    {
        return tables[(size_t) juce::jlimit (0, kNumShapes - 1, shape)].data();
    }

    /** One clamp + one table lookup + one lerp. No transcendental. */
    float readAt (const float* table, float phase) const noexcept
    {
        const float p    = juce::jlimit (0.0f, 1.0f, phase);
        const float fpos = p * static_cast<float> (size - 1);
        const int   i0   = static_cast<int> (fpos);
        const int   i1   = juce::jmin (i0 + 1, size - 1);
        const float frac = fpos - static_cast<float> (i0);

        return table[i0] + frac * (table[i1] - table[i0]);
    }

    /** Convenience read for non-hot paths (tests, offline analysis). */
    float read (int shape, float phase) const noexcept
    {
        return readAt (getTable (shape), phase);
    }

    //==========================================================================
    // Tilt warp. Latched per grain at spawn — a mid-flight tilt change must
    // never re-shape a live grain's envelope, which is the same click-free
    // mechanism delayTime/grainSize/density/width already use.

    struct Tilt
    {
        float t = 0.5f;   // peak (midpoint) position within the grain
        float a = 1.0f;   // 0.5 / t
        float b = 1.0f;   // 0.5 / (1 - t)

        /** q = min(p,t)·a + max(p-t,0)·b. At t = 0.5 this is BITWISE p. */
        float warp (float p) const noexcept
        {
            return juce::jmin (p, t) * a + juce::jmax (p - t, 0.0f) * b;
        }
    };

    /** peakPos 0.5 returns { 0.5, 1.0f, 1.0f } with both divisions exact, which
        is what makes the default path the bitwise identity. */
    static Tilt makeTilt (float peakPos) noexcept
    {
        const float t = juce::jlimit (kMinPeakPos, kMaxPeakPos, peakPos);
        return { t, 0.5f / t, 0.5f / (1.0f - t) };
    }

    //==========================================================================
    // Power normalisation. Both are pure reads of constants computed in build();
    // safe to call at block rate, never allocating.

    /** sqrt(m_hann / m_shape) — folded into grainGain so a shape change is a
        timbre change and not a level change. Exactly 1.0f for Hann (a value
        divided by itself, then sqrt(1.0f)). */
    float getShapeNorm (int shape) const noexcept
    {
        return stats[(size_t) juce::jlimit (0, kNumShapes - 1, shape)].shapeNorm;
    }

    /** sqrt(m / (t·mLo + (1-t)·mHi)) — exactly 1.0f whenever mLo == mHi (all
        four symmetric shapes) or t == 0.5 (the default), because the denominator
        is then bitwise the numerator: 0.5·mLo + 0.5·mHi and 0.5·(mLo + mHi)
        round identically (halving is exact). */
    float getTiltNorm (int shape, float peakPos) const noexcept
    {
        const auto& s = stats[(size_t) juce::jlimit (0, kNumShapes - 1, shape)];
        const float t = juce::jlimit (kMinPeakPos, kMaxPeakPos, peakPos);

        const float warped = t * s.meanSqLo + (1.0f - t) * s.meanSqHi;

        if (warped <= 0.0f)
            return 1.0f;

        return std::sqrt (s.meanSq / warped);
    }

    /** Feedback-tap trim. Multiplies the LOOP gains only — never the output
        gains — so that the loop's per-generation amplitude is the same for
        every shape and tilt.

        ── Why the loop needs a DIFFERENT constant from the output ────────────
        getShapeNorm/getTiltNorm normalise POWER, which is correct for the
        output: a first pass through broadband input has each grain reading a
        different stretch of the capture ring, those contributions are
        essentially decorrelated, and decorrelated things sum in power. Probe Z2
        measures exactly that and finds all five shapes within 0.15 dB.

        The feedback path sums something else. What recirculates is the wash the
        engine just produced — self-similar material that overlapping grains read
        at nearby offsets — so it sums closer to COHERENTLY, and a coherent sum
        follows the window's mean, not its mean square. Normalising the loop on
        power therefore leaves a real per-generation error: measured across the
        five shapes at feedback = 100 it spanned 5.7 dB/s of decay rate, with the
        ranking tracking the shapes' amplitude duty exactly. A control sold as
        "window shape" would have been heard as "how long the tail lasts", which
        is the second half of what this release was asked not to do.

        Both laws are real; they just apply to different sums. The engine has
        carried separate output and feedback-tap gains since v1.1.0 (built for
        gainRandom), so each path can take the normalisation its own summing law
        needs. Exactly 1.0f at (Hann, 0.5): a value divided by itself. */
    float getLoopNorm (int shape, float peakPos) const noexcept
    {
        const auto& s = stats[(size_t) juce::jlimit (0, kNumShapes - 1, shape)];
        const float t = juce::jlimit (kMinPeakPos, kMaxPeakPos, peakPos);

        const float windowNorm = getShapeNorm (shape) * getTiltNorm (shape, peakPos);
        const float effMean    = t * s.meanLo + (1.0f - t) * s.meanHi;
        const float denom      = windowNorm * effMean;

        if (denom <= 0.0f)
            return 1.0f;

        return stats[(size_t) hann].mean / denom;
    }

    /** Mean square (power duty cycle) of a shape's window. Reported by the
        render harness so the normalisation constants are printed numbers rather
        than a claim in a comment. */
    float getMeanSquare (int shape) const noexcept
    {
        return stats[(size_t) juce::jlimit (0, kNumShapes - 1, shape)].meanSq;
    }

    /** Mean (amplitude duty cycle) — the constant the feedback path follows. */
    float getMean (int shape) const noexcept
    {
        return stats[(size_t) juce::jlimit (0, kNumShapes - 1, shape)].mean;
    }

    int getSize() const noexcept { return size; }

private:
    //==========================================================================
    void build()
    {
        for (auto& t : tables)
            t.assign ((size_t) size, 0.0f);

        const float twoPi = juce::MathConstants<float>::twoPi;
        const float pi    = juce::MathConstants<float>::pi;

        // Gaussian: σ and the pedestal it must be lifted off (see header note).
        constexpr float kSigma = 0.18f;
        const float gaussEdge  = std::exp (-0.5f * (0.5f / kSigma) * (0.5f / kSigma));
        const float gaussScale = 1.0f / (1.0f - gaussEdge);

        // Tukey: 50 % taper → cosine over the outer quarter at each end.
        constexpr float kTukeyTaper = 0.5f;
        const float taperEnd = kTukeyTaper * 0.5f;          // 0.25

        // Expo-Decay: smooth attack fraction, then exponential to exactly zero.
        constexpr float kAttack = 0.02f;
        constexpr float kDecayK = 5.0f;
        const float expEnd   = std::exp (-kDecayK);
        const float expScale = 1.0f / (1.0f - expEnd);

        for (int i = 0; i < size; ++i)
        {
            const float phi = static_cast<float> (i) / static_cast<float> (size - 1);   // 0..1 inclusive

            // 0 — Hann. Bit-for-bit the v1.0.0 expression: this table IS the
            // shipped window and any drift here changes every existing session.
            tables[(size_t) hann][(size_t) i] = 0.5f * (1.0f - std::cos (twoPi * phi));

            // 1 — Tukey (r = 0.5): raised-cosine taper, flat middle half.
            {
                float w;
                if (phi < taperEnd)
                    w = 0.5f * (1.0f - std::cos (pi * phi / taperEnd));
                else if (phi > 1.0f - taperEnd)
                    w = 0.5f * (1.0f - std::cos (pi * (1.0f - phi) / taperEnd));
                else
                    w = 1.0f;

                tables[(size_t) tukey][(size_t) i] = w;
            }

            // 2 — Gaussian, pedestal removed so the ends reach exactly zero.
            {
                const float d = (phi - 0.5f) / kSigma;
                const float g = std::exp (-0.5f * d * d);
                tables[(size_t) gaussian][(size_t) i] = juce::jmax (0.0f, (g - gaussEdge) * gaussScale);
            }

            // 3 — Triangular.
            tables[(size_t) triangular][(size_t) i] = 1.0f - std::abs (2.0f * phi - 1.0f);

            // 4 — Expo-Decay: raised-cosine attack (so the grain's leading edge
            // is not a step) into an exponential that lands on zero at φ = 1.
            {
                float w;
                if (phi < kAttack)
                {
                    w = 0.5f * (1.0f - std::cos (pi * phi / kAttack));
                }
                else
                {
                    const float u = (phi - kAttack) / (1.0f - kAttack);
                    w = (std::exp (-kDecayK * u) - expEnd) * expScale;
                }

                tables[(size_t) expoDecay][(size_t) i] = juce::jmax (0.0f, w);
            }
        }

        computeStats();
    }

    /** Half-window mean squares, integrated from the tables themselves rather
        than from closed forms — the Gaussian's pedestal removal and the
        Expo-Decay's piecewise definition have no tidy analytic mean square, and
        a hand-derived constant that drifts from the table is exactly the class
        of silent error this normalisation exists to prevent.

        size is even in every real configuration (2048), so the two halves hold
        the same number of entries and meanSq is their exact average. */
    void computeStats()
    {
        const int half = size / 2;

        for (int s = 0; s < kNumShapes; ++s)
        {
            const auto& t = tables[(size_t) s];

            // Both the POWER duty (mean of w²) and the AMPLITUDE duty (mean of
            // w) are needed: the output path sums decorrelated grains and
            // follows the first, the feedback path sums self-similar material
            // and follows the second. See getLoopNorm().
            double lo = 0.0, hi = 0.0, aLo = 0.0, aHi = 0.0;

            for (int i = 0; i < half; ++i)
            {
                const double w = (double) t[(size_t) i];
                lo  += w * w;
                aLo += w;
            }

            for (int i = half; i < size; ++i)
            {
                const double w = (double) t[(size_t) i];
                hi  += w * w;
                aHi += w;
            }

            const double nLo = (double) juce::jmax (1, half);
            const double nHi = (double) juce::jmax (1, size - half);

            auto& st = stats[(size_t) s];
            st.meanSqLo = (float) (lo  / nLo);
            st.meanSqHi = (float) (hi  / nHi);
            st.meanSq   = 0.5f * (st.meanSqLo + st.meanSqHi);
            st.meanLo   = (float) (aLo / nLo);
            st.meanHi   = (float) (aHi / nHi);
            st.mean     = 0.5f * (st.meanLo + st.meanHi);

            // ── canonicalise a symmetric window's two halves ────────────────
            // Hann, Tukey, Gaussian and Triangular are symmetric AS FUNCTIONS,
            // but their TABLES are not symmetric to the last bit: w[i] and
            // w[size-1-i] come from std::cos/std::exp evaluated at different
            // arguments, so the two halves' summed power can disagree in the
            // last ulp or two. Left alone that turns the tilt normalisation from
            // "exactly 1.0f at every tilt" into "1.0f ± 5e-8", which is
            // inaudible but destroys the ability to ASSERT power invariance as
            // an exact property — and an invariant that can only be checked to
            // within a tolerance is one that can rot by a real amount later
            // without any probe noticing.
            //
            // A relative 1e-6 gate separates a float artefact from a genuinely
            // asymmetric window by an enormous margin: Expo-Decay's two halves
            // differ by a factor of about 11, i.e. by 1e7 times this threshold.
            // Mirroring the table itself would be the other fix and is NOT
            // available: the Hann table must stay bit-identical to the one
            // v1.0.0 built, or every existing session changes.
            if (std::abs (st.meanSqLo - st.meanSqHi)
                  <= 1.0e-6f * juce::jmax (st.meanSqLo, st.meanSqHi))
                st.meanSqLo = st.meanSqHi = st.meanSq;

            if (std::abs (st.meanLo - st.meanHi)
                  <= 1.0e-6f * juce::jmax (st.meanLo, st.meanHi))
                st.meanLo = st.meanHi = st.mean;
        }

        // Hann is the reference: its own norm is a value divided by itself, so
        // sqrt() of exactly 1.0f, so exactly 1.0f — the multiply into grainGain
        // is then a bitwise no-op and the shipped render is preserved.
        const float ref = stats[(size_t) hann].meanSq;

        for (auto& st : stats)
            st.shapeNorm = (st.meanSq > 0.0f) ? std::sqrt (ref / st.meanSq) : 1.0f;
    }

    struct ShapeStats
    {
        float meanSqLo  = 0.0f;   // mean of w² over [0, 0.5)
        float meanSqHi  = 0.0f;   // mean of w² over [0.5, 1]
        float meanSq    = 0.0f;   // 0.5·(lo + hi) — the POWER duty (output path)
        float meanLo    = 0.0f;   // mean of w  over [0, 0.5)
        float meanHi    = 0.0f;   // mean of w  over [0.5, 1]
        float mean      = 0.0f;   // 0.5·(lo + hi) — the AMPLITUDE duty (loop path)
        float shapeNorm = 1.0f;   // sqrt(m_hann / m)
    };

    int size;
    std::array<std::vector<float>, kNumShapes> tables {};
    std::array<ShapeStats,         kNumShapes> stats  {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WindowLut)
};
