/*
   This file is part of O-ReverseDelay, an Ouaricon Audio plugin.
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
        1 = Tukey       cosine taper α, flat middle          — the "open" one
        2 = Gaussian    σ = 0.18, PEDESTAL REMOVED (below)
        3 = Triangular  1 - |2φ - 1|
        4 = Expo-Decay  smooth 2 % attack, then e^-5u to zero — "plucked"

    ── v1.4.0: Tukey's taper α becomes continuous ─────────────────────────────
    α was frozen at 0.5 (a 50 % taper) from v1.2.0. It is now a parameter over
    [0.01, 1.0]: 0.01 is very nearly rectangular — a fast edge, an "open"/gated
    grain — and 1.0 IS Hann, reached exactly rather than approached.

    It is rendered WITHOUT a new table and WITHOUT a transcendental in the grain
    loop, because Tukey's taper is literally a Hann half. With taperEnd = α/2,

        Tukey_taper(φ) = 0.5(1 - cos(πφ / taperEnd))
        Hann(x)        = 0.5(1 - cos(2πx))
        -> Tukey_taper(φ) = Hann(φ / (2·taperEnd))

    so the whole window is one remap of the phase into the EXISTING Hann table,
    saturating at the flat top (Hann(0.5) = 1 exactly):

        u = min(φ, 1 - φ)                    distance to the nearest edge
        r = min(u / taperEnd, 1) · 0.5       [0, 0.5], flat top at exactly 0.5
        w = Hann_table(r)

    Branch-free apart from one per-grain flag, no extra memory, and continuous in
    α rather than a quantised bank of tables.

    ⚠ This makes the Tukey render differ from v1.3.0's stored Tukey table by up
    to 2.4e-6 (−112.5 dB, measured by harness probe AH) — the Hann table's
    linear-interpolation error, not a change of shape. It is NOT bitwise, and
    cannot be: reading a 2048-point table at an arbitrary phase is not the same
    operation as evaluating cos there.

    On the size of that number, stated properly rather than waved at: 2.4e-6 is
    roughly 20x a 24-bit LSB, so it is NOT "below the noise floor" as an absolute
    envelope error. What makes it inaudible is where it occurs — the worst case is
    at φ ≈ 0.999, i.e. at the very end of the taper where the window value is
    itself almost zero, so the error multiplies a sample that is being faded out.
    Against a typical source level it lands near −124 dB at the output. The
    deviation also affects TUKEY ONLY: Hann, Gaussian, Triangular and Expo-Decay
    never take this path, all eight factory presets are on Hann, and a default
    session is bitwise unchanged. At α = 1.0 the remap lands on the table's own
    points and the deviation drops to 4.2e-7.

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

    //==========================================================================
    // v1.4.0 — Tukey taper α.
    //
    // Range and STEP are both load-bearing, and the step is the interesting one.
    // α's power and amplitude duty cycles change with α, so shapeNorm and
    // loopNorm must track it — and this file's rule is that those constants are
    // integrated from the actual window, never hand-derived, because a closed
    // form that drifts from the window is the exact silent error the
    // normalisation exists to prevent.
    //
    // Integrating 2048 points per block on the audio thread is not available, so
    // the stats are precomputed per α in the constructor. Choosing a 0.01 step
    // over [0.01, 1.00] makes that a 100-entry grid on which EVERY reachable α
    // lands exactly — so the stats are exact rather than interpolated, and the
    // α = 0.5 entry is built by the same expression v1.2.0 used, which is what
    // keeps the shipped normalisation constants bitwise unchanged.
    //
    // 100 positions on a taper knob is far finer than audible; the alternative
    // (a continuous α with interpolated stats) would buy nothing and would cost
    // the exactness at the default.
    static constexpr float kTukeyTaperMin     = 0.01f;
    static constexpr float kTukeyTaperMax     = 1.00f;
    static constexpr float kTukeyTaperStep    = 0.01f;
    static constexpr float kTukeyTaperDefault = 0.50f;   // v1.2.0's frozen value

    static constexpr int kNumTaperSteps = 100;           // α = (i + 1)/100

    /** α -> grid index. Rounds rather than truncates: the parameter is snapped to
        the same 0.01 grid by its NormalisableRange, so this is exact for every
        value the parameter can hold, and rounding keeps it exact against the
        float representation of e.g. 0.5f rather than landing on 49 vs 50. */
    static int taperIndex (float alpha) noexcept
    {
        const float a = juce::jlimit (kTukeyTaperMin, kTukeyTaperMax, alpha);
        return juce::jlimit (0, kNumTaperSteps - 1,
                             juce::roundToInt (a * 100.0f) - 1);
    }

    static float taperAlphaAt (int index) noexcept
    {
        return static_cast<float> (juce::jlimit (0, kNumTaperSteps - 1, index) + 1) * 0.01f;
    }

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
    // v1.4.0 — the Tukey taper remap.

    /** Per-grain latched taper geometry. `active` is false for every shape except
        Tukey, and for Tukey it is always true — so it is constant for the whole
        life of a grain and the branch in readShaped() is perfectly predicted.

        A branch-free form (`r = q + active·(map(q) − q)`) was considered and
        rejected: it would evaluate the remap for all five shapes to avoid a
        branch that never mispredicts. */
    struct Taper
    {
        bool  active      = false;
        float invTaperEnd = 0.0f;   // 1 / (α/2)
    };

    /** Resolved once per block and copied into each grain at spawn, exactly like
        Tilt. Non-Tukey shapes get the inactive geometry, so a stale α cannot
        reach a shape that does not use it. */
    static Taper makeTaper (int shape, float alpha) noexcept
    {
        if (shape != tukey)
            return {};

        const float a  = juce::jlimit (kTukeyTaperMin, kTukeyTaperMax, alpha);
        const float te = a * 0.5f;

        return { true, 1.0f / te };
    }

    /** The hot read. `table` must be the HANN table when tp.active — the caller
        resolves that once per grain, because Tukey no longer has a table of its
        own to read (see the header note: its taper IS a Hann half).

        The flat top is exact, not approximate: min(...) saturates at 1.0f, times
        0.5f is exactly 0.5f, and Hann(0.5) is exactly 1.0f — so a grain's
        plateau is a true unity plateau and not 0.9999. */
    float readShaped (const float* table, const Taper& tp, float phase) const noexcept
    {
        if (! tp.active)
            return readAt (table, phase);

        const float p = juce::jlimit (0.0f, 1.0f, phase);
        const float u = juce::jmin (p, 1.0f - p);

        return readAt (table, juce::jmin (u * tp.invTaperEnd, 1.0f) * 0.5f);
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
        return statsFor (shape, kTukeyTaperDefault).shapeNorm;
    }

    /** α-aware overload. Tukey's duty cycle is a strong function of α — mean
        square runs 0.994 at α = 0.01 down to 0.375 at α = 1.0, so the level
        correction spans 4.2 dB across the knob. Without this the taper control
        would be heard as a volume control, which is precisely what v1.2.0's
        getShapeNorm exists to stop `grainShape` from being.

        Every other shape ignores α. */
    float getShapeNorm (int shape, float alpha) const noexcept
    {
        return statsFor (shape, alpha).shapeNorm;
    }

    /** sqrt(m / (t·mLo + (1-t)·mHi)) — exactly 1.0f whenever mLo == mHi (all
        four symmetric shapes) or t == 0.5 (the default), because the denominator
        is then bitwise the numerator: 0.5·mLo + 0.5·mHi and 0.5·(mLo + mHi)
        round identically (halving is exact). */
    float getTiltNorm (int shape, float peakPos) const noexcept
    {
        return getTiltNorm (shape, peakPos, kTukeyTaperDefault);
    }

    /** α-aware overload. Still exactly 1.0f for Tukey at EVERY α, because Tukey
        stays symmetric as α varies — its two half-window mean squares agree to
        the last bit at 0.01 and at 1.0 alike, so the canonicalisation in
        computeStats() collapses them and the ratio is a value over itself. That
        is worth stating because it is not automatic: an α that broke symmetry
        would silently turn tilt into a level control. */
    float getTiltNorm (int shape, float peakPos, float alpha) const noexcept
    {
        const auto& s = statsFor (shape, alpha);
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
        return getLoopNorm (shape, peakPos, kTukeyTaperDefault);
    }

    /** α-aware overload. The loop follows the AMPLITUDE duty, which moves with α
        on its own schedule (mean runs 0.995 -> 0.500, i.e. 6.0 dB, against the
        power duty's 4.2 dB) — so a single α-aware constant on the output would
        NOT have fixed the loop, exactly as v1.2.0 found for shape and v1.3.0
        found for overlap. Third time the two paths needed different α-dependent
        constants; the split earns its keep again. */
    float getLoopNorm (int shape, float peakPos, float alpha) const noexcept
    {
        const auto& s = statsFor (shape, alpha);
        const float t = juce::jlimit (kMinPeakPos, kMaxPeakPos, peakPos);

        const float windowNorm = getShapeNorm (shape, alpha)
                               * getTiltNorm  (shape, peakPos, alpha);
        const float effMean    = t * s.meanLo + (1.0f - t) * s.meanHi;
        const float denom      = windowNorm * effMean;

        if (denom <= 0.0f)
            return 1.0f;

        return stats[(size_t) hann].mean / denom;
    }

    /** v1.6.0 — the FORWARD-grain output trim, for the `direction` blend.

        Applied to the OUTPUT pan gains of forward grains only, and to nothing
        else. Third distinct summing law this file has had to serve, and it is
        worth stating why a third one exists rather than reusing either of the
        two above.

        ── Forward grains sum COHERENTLY, and not approximately ────────────────
        A grain latches `readAbs = s − gD` at its spawn sample s and steps its
        read by ±1 while the write head advances +1. At output time t a grain's
        own index is n = t − s, so it reads

            reverse (step −1):   s − gD − n  =  2s − gD − t     depends on s
            forward (step +1):   s − gD + n  =       t − gD     does NOT

        Every forward grain in flight therefore reads the SAME source sample,
        exactly. Not "self-similar material at nearby offsets" the way the
        feedback loop is — bit-for-bit the same sample. So the forward set adds
        in AMPLITUDE (N·m) where the reverse set adds in POWER (sqrt(N·q)), and
        at overlap 8 with Hann that is a 7.3 dB difference. Uncorrected, a knob
        sold as "direction" would be heard first as a volume knob — the same
        failure getShapeNorm (v1.2.0), loopCountTrim (v1.3.0) and the α-aware
        overloads (v1.4.0) each exist to prevent.

        This is also, incidentally, why the plugin is a REVERSE delay: the
        s-dependence that decorrelates the grains is created by the read head
        moving opposite to the write head. A unit-rate forward read is a plain
        delay tap by construction, at any overlap. Per-grain decorrelation on
        the forward side is available — it is what `delayScatter` does, since a
        scattered grain latches a different gD and therefore reads a different
        point — but it is a user control, not a property of this law.

        ── The constant, derived ───────────────────────────────────────────────
        The grain's shared gain already carries 1/sqrt(N) · windowNorm, and
        windowNorm is sqrt(q_hann / q_eff) by construction, so

            reverse level = sqrt(N·q_eff) · (1/sqrt(N)) · windowNorm = sqrt(q_hann)
            forward level = (N·m_eff)     · (1/sqrt(N)) · windowNorm · k

        Setting the two equal gives k = sqrt(q_eff / N) / m_eff, with q_eff and
        m_eff the TILT-WARPED, α-aware duties — the same two effective moments
        getTiltNorm and getLoopNorm already form. Derived, not fitted.

        ── Why the LOOP takes no direction trim ────────────────────────────────
        Because the loop's model is already the coherent one. getLoopNorm exists
        precisely because what recirculates sums closer to amplitude than to
        power, so a forward grain and a reverse grain contribute identically per
        grain through the feedback tap and the per-generation gain does not move
        with the direction mix. Probe AM measures that rather than assuming it. */
    float getForwardNorm (int shape, float peakPos, float alpha, float overlap) const noexcept
    {
        const auto& s = statsFor (shape, alpha);
        const float t = juce::jlimit (kMinPeakPos, kMaxPeakPos, peakPos);

        const float effMeanSq = t * s.meanSqLo + (1.0f - t) * s.meanSqHi;
        const float effMean   = t * s.meanLo   + (1.0f - t) * s.meanHi;

        if (effMean <= 0.0f || effMeanSq <= 0.0f || overlap <= 0.0f)
            return 1.0f;

        return std::sqrt (effMeanSq / overlap) / effMean;
    }

    /** Mean square (power duty cycle) of a shape's window. Reported by the
        render harness so the normalisation constants are printed numbers rather
        than a claim in a comment. */
    float getMeanSquare (int shape) const noexcept
    {
        return statsFor (shape, kTukeyTaperDefault).meanSq;
    }

    float getMeanSquare (int shape, float alpha) const noexcept
    {
        return statsFor (shape, alpha).meanSq;
    }

    /** Mean (amplitude duty cycle) — the constant the feedback path follows. */
    float getMean (int shape) const noexcept
    {
        return statsFor (shape, kTukeyTaperDefault).mean;
    }

    float getMean (int shape, float alpha) const noexcept
    {
        return statsFor (shape, alpha).mean;
    }

    int getSize() const noexcept { return size; }

    /** Samples the window a caller will actually HEAR, composing shape, tilt and
        taper in the same order and by the same calls the render loop uses.

        Exists for the v1.4.0 UI display, and deliberately not reimplemented in
        JavaScript. A JS copy of this composition would be a second definition of
        the window free to drift from the first, and the drift would be invisible
        — the graph would keep looking plausible while no longer describing the
        audio. Same reasoning that keeps knob readouts on getScaledValue() instead
        of a JS range table.

        Message thread only (it is neither RT-safe nor RT-needed): one call fills
        a whole curve. */
    void sampleWindow (int shape, float peakPos, float alpha,
                       float* dest, int numPoints) const noexcept
    {
        if (dest == nullptr || numPoints <= 0)
            return;

        const auto  tilt  = makeTilt (peakPos);
        const auto  taper = makeTaper (shape, alpha);
        const float* table = taper.active ? getTable (hann) : getTable (shape);

        for (int i = 0; i < numPoints; ++i)
        {
            const float p = numPoints == 1
                              ? 0.0f
                              : static_cast<float> (i) / static_cast<float> (numPoints - 1);

            dest[i] = readShaped (table, taper, tilt.warp (p));
        }
    }

private:
    //==========================================================================
    // Declared before the methods that return a reference to it — a member
    // function's RETURN TYPE must be known at its declaration, unlike its body.
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

            // 1 — Tukey at the DEFAULT α: raised-cosine taper, flat middle half.
            //
            // v1.4.0 no longer RENDERS from this table — a variable α is served by
            // remapping into the Hann table (header note) — but it is not dead
            // code. It is the independent reference that harness probe AH measures
            // the remap against: built from std::cos directly, so a probe
            // comparing the two is comparing the remap to real trigonometry
            // rather than to itself.
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
        computeTaperStats();
    }

    /** v1.4.0 — the per-α Tukey stats grid.
        One entry per reachable α, each integrated from the window the render loop
        will ACTUALLY produce at that α (readShaped through the Hann table), not
        from the analytic Tukey and not from a closed form. That is the same
        principle computeStats() follows, applied to a parameterised shape: the
        constants describe what is heard, by construction, so they cannot drift
        from it.

        Cost is 100 x `size` table reads, once, in the constructor.

        The closed forms — meanSq = 1 − 0.625α and mean = 1 − 0.5α — are real and
        exact for the continuous window, and are deliberately NOT used here. They
        are asserted against this grid by harness probe AI instead, which keeps
        them as an auditable cross-check rather than an unverifiable claim. */
    void computeTaperStats()
    {
        const int half = size / 2;
        std::vector<float> w ((size_t) size, 0.0f);

        for (int k = 0; k < kNumTaperSteps; ++k)
        {
            const auto taper = makeTaper (tukey, taperAlphaAt (k));

            for (int i = 0; i < size; ++i)
            {
                const float phi = static_cast<float> (i) / static_cast<float> (size - 1);
                w[(size_t) i] = readShaped (tables[(size_t) hann].data(), taper, phi);
            }

            double lo = 0.0, hi = 0.0, aLo = 0.0, aHi = 0.0;

            for (int i = 0; i < half; ++i)
            {
                const double v = (double) w[(size_t) i];
                lo += v * v;  aLo += v;
            }

            for (int i = half; i < size; ++i)
            {
                const double v = (double) w[(size_t) i];
                hi += v * v;  aHi += v;
            }

            const double nLo = (double) juce::jmax (1, half);
            const double nHi = (double) juce::jmax (1, size - half);

            auto& st = taperStats[(size_t) k];
            st.meanSqLo = (float) (lo  / nLo);
            st.meanSqHi = (float) (hi  / nHi);
            st.meanSq   = 0.5f * (st.meanSqLo + st.meanSqHi);
            st.meanLo   = (float) (aLo / nLo);
            st.meanHi   = (float) (aHi / nHi);
            st.mean     = 0.5f * (st.meanLo + st.meanHi);

            // Same canonicalisation as computeStats(), and needed for the same
            // reason: Tukey is symmetric at every α, but its two halves are built
            // from table reads at different arguments and can disagree in the last
            // ulp. Collapsing them is what keeps getTiltNorm EXACTLY 1.0f for
            // Tukey at every α — an invariant that can only be checked to within
            // a tolerance is one that can rot later without any probe noticing.
            if (std::abs (st.meanSqLo - st.meanSqHi)
                  <= 1.0e-6f * juce::jmax (st.meanSqLo, st.meanSqHi))
                st.meanSqLo = st.meanSqHi = st.meanSq;

            if (std::abs (st.meanLo - st.meanHi)
                  <= 1.0e-6f * juce::jmax (st.meanLo, st.meanHi))
                st.meanLo = st.meanHi = st.mean;

            st.shapeNorm = (st.meanSq > 0.0f)
                             ? std::sqrt (stats[(size_t) hann].meanSq / st.meanSq)
                             : 1.0f;
        }
    }

    /** Routes Tukey to its α grid and every other shape to the fixed stats. The
        single choke point for "does α matter here", so no accessor can forget. */
    const ShapeStats& statsFor (int shape, float alpha) const noexcept
    {
        const int s = juce::jlimit (0, kNumShapes - 1, shape);

        if (s == tukey)
            return taperStats[(size_t) taperIndex (alpha)];

        return stats[(size_t) s];
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

    int size;
    std::array<std::vector<float>, kNumShapes> tables {};
    std::array<ShapeStats,         kNumShapes> stats  {};

    /** v1.4.0 — Tukey's stats per reachable α. 100 x 7 floats = 2.8 KB, built in
        the constructor. Indexed by taperIndex(), never interpolated: every α the
        parameter can hold lands exactly on an entry. */
    std::array<ShapeStats, kNumTaperSteps> taperStats {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WindowLut)
};
