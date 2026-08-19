/*
   This file is part of O-Bitrot, an Ouaricon Audio plugin.
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

    O-Bitrot - WowFlutter (v1.4.0, improvement brief item 3)

    The tape family's continuous speed-modulation bed. Between events the
    engine used to be bit-clean — TapeTransport returns exactly 1.0 while
    Idle, and nothing else modulated the transport — so a "worn cassette"
    sounded like a perfect deck that occasionally broke. Real decks wow
    (0.5-6 Hz, capstan/pinch-roller eccentricity) and flutter (6-100 Hz,
    bearing/scrape) ALL the time: 0.1-1% WRMS on consumer gear, several
    percent when dying.

    WHY THIS MODULATES A READ OFFSET AND NOT THE RATE
    -------------------------------------------------
    The obvious implementation — multiply the per-sample read rate — breaks
    two load-bearing invariants of this engine:

      1. ReadHead's lag-overflow clamp is SUPPRESSED while a CD loop or a
         locked groove owns the read rate, and the proof that the suppression
         is safe is "such a loop holds rate at EXACTLY 1.0, so lag is constant
         between its own gated jumps". A rate multiplier falsifies the
         premise; the clamp would have to be re-armed, and lag would drift
         under every loop.
      2. The engine's steady state is lag 0 — the ring is written before it is
         read, so the read head normally sits on the newest sample. Any rate
         above 1.0 there drives `pos` straight into ReadHead's `pos > next`
         pin, which is a zero-order hold. At 1.5% deviation and 0.7 Hz that is
         ~8 ms of held sample every wow cycle: not wow, just a stutter.

    Modulating a READ OFFSET instead is the same physics from the other end.
    Pitch deviation is the derivative of the delay, so a lag excursion of
    amplitude A at frequency f IS a rate deviation of A*pi*f/fs — this class
    inverts that relation and specifies each partial by the deviation it
    should produce. The offset is >= 0 by construction (each partial is a
    raised cosine), so the head never crosses the write head; `pos`, the lag
    budgets, the loop-wrap tests and the clamp proof are all untouched. It is
    the classic flutter-delay structure, and it composes with everything.

    BOUNDED BY CONSTRUCTION
    -----------------------
    Filtered noise would need a runtime normalisation to hit a stated "% of
    deviation", so the bed is three quasi-periodic partials instead — which
    is also what the mechanism is: two fixed-frequency wow partials (capstan
    eccentricity is periodic, once per revolution) and one flutter partial
    whose frequency random-walks across 7-55 Hz. Per-partial amplitudes drift
    slowly on the dedicated `wow` RNG stream so the bed never loops audibly.

    Every rate-of-change term is deliberately slow, because each one shows up
    in the output as pitch: the amplitude and frequency walks are one-poled
    over seconds, and DEPTH itself ramps over kDepthRampSeconds. A 20 ms depth
    fade would have swung 5 ms of delay in 20 ms — a 25% pitch jump on knob
    touch.

    At depth exactly 0 the offset is exactly 0.0, and `pos - 0.0` is
    bit-identical to `pos`, so CaptureRing's exact-integer fast path still
    fires and the all-off passthrough stays bit-transparent (FUNC-02). Any
    depth above 0 intentionally leaves that fast path — the bed IS a
    fractional read.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class WowFlutter
{
public:
    // Peak read-rate deviation at TAPE_WOW = 100%, summed across partials.
    // 2% is the "dying deck" end of the research range (0.1-1% WRMS is
    // healthy consumer gear); the partials rarely align, so the typical
    // excursion is well under this.
    static constexpr double kMaxDeviation     = 0.020;

    // Depth fade. Long ON PURPOSE — see the header note: this ramp multiplies
    // a delay of several milliseconds, and its slope is pitch.
    static constexpr double kDepthRampSeconds = 3.0;

    // Partial amplitudes are redrawn on this fixed sample counter and one-pole
    // smoothed toward the new target. Both are slow for the same reason.
    static constexpr double kRedrawSeconds    = 0.5;
    static constexpr double kAmpTauSeconds    = 2.0;
    static constexpr double kFreqTauSeconds   = 1.5;

    static constexpr double kFlutterMinHz     = 7.0;
    static constexpr double kFlutterMaxHz     = 55.0;

    void prepare (double sampleRate)
    {
        fs = sampleRate;

        depthStep     = 1.0 / juce::jmax (1.0, kDepthRampSeconds * fs);
        redrawSamples = juce::jmax (1, static_cast<int> (kRedrawSeconds * fs));
        ampCoeff      = 1.0 - std::exp (-1.0 / juce::jmax (1.0, kAmpTauSeconds * fs));
        freqCoeff     = 1.0 - std::exp (-1.0 / juce::jmax (1.0, kFreqTauSeconds * fs));

        // Deviation budget across the three partials. The two wow partials
        // carry three quarters of it (wow is the audible one); flutter is the
        // fast shimmer on top.
        partials[0] = { 0.0, 0.73, 0.73, 1.0, 1.0, 0.010, false };  // wow, capstan-ish
        partials[1] = { 0.0, 2.31, 2.31, 1.0, 1.0, 0.005, false };  // wow, roller-ish
        partials[2] = { 0.0, 19.0, 19.0, 1.0, 1.0, 0.005, true  };  // flutter, walks

        // Asserted against the ACTUAL table rather than restated as a mirrored
        // constant, so the budget cannot drift away from the partials it
        // describes (pattern_test_fixture_mirrors_drift_silently).
        double budget = 0.0;
        for (const auto& p : partials)
            budget += p.deviation;

        jassert (budget <= kMaxDeviation + 1.0e-12);
        juce::ignoreUnused (budget);

        reset();
    }

    void reset() noexcept
    {
        depth        = 0.0;
        depthTarget  = 0.0;
        redrawT      = 0;

        for (auto& p : partials)
        {
            p.phase     = 0.0;
            p.amp       = 1.0;
            p.ampTarget = 1.0;
            p.freq      = p.baseFreq;
        }
    }

    /** Per-block snapshot, in 0..1. Pass 0 when the tape family is disabled —
        the bed is a tape artifact, and it fades out over the same slow ramp
        rather than stepping the delay to zero. */
    void setDepth (double depth01) noexcept
    {
        depthTarget = juce::jlimit (0.0, 1.0, depth01);
    }

    /** True while the bed contributes EXACTLY nothing — the condition under
        which the read is still on CaptureRing's bit-exact integer path. */
    bool isTransparent() const noexcept { return depth == 0.0; }

    /** Per-sample read offset in samples, always >= 0. The oscillators and the
        RNG schedule run unconditionally so the bed is phase-continuous across
        depth changes; only the final scale is gated. */
    double nextOffsetSamples (juce::Random& rng) noexcept
    {
        // Slow parameter walk on the dedicated stream: 4 draws every
        // kRedrawSeconds, on a sample counter (block-size invariant).
        if (--redrawT <= 0)
        {
            redrawT = redrawSamples;

            for (auto& p : partials)
                p.ampTarget = 0.5 + 0.5 * rng.nextDouble();

            flutterFreqTarget = kFlutterMinHz
                                + (kFlutterMaxHz - kFlutterMinHz) * rng.nextDouble();
        }

        double total = 0.0;

        for (auto& p : partials)
        {
            p.amp += ampCoeff * (p.ampTarget - p.amp);

            // Only the flutter partial's frequency walks; the wow partials are
            // capstan-periodic and hold their base frequency.
            const double freqTarget = p.walks ? flutterFreqTarget : p.baseFreq;
            p.freq += freqCoeff * (freqTarget - p.freq);

            p.phase += juce::MathConstants<double>::twoPi * p.freq / fs;
            if (p.phase >= juce::MathConstants<double>::twoPi)
                p.phase -= juce::MathConstants<double>::twoPi;

            // deviation -> lag amplitude: a raised cosine of peak-to-peak L at
            // frequency f has peak slope L*pi*f, i.e. a rate deviation of
            // L*pi*f/fs. Inverting keeps the deviation constant as the flutter
            // partial's frequency walks (holding L fixed instead would make it
            // 8x deeper at 55 Hz than at 7 Hz).
            const double lagAmp = p.deviation * fs
                                  / (juce::MathConstants<double>::pi * juce::jmax (0.05, p.freq));

            total += p.amp * lagAmp * 0.5 * (1.0 - std::cos (p.phase));
        }

        // Depth ramp, landing exactly on the target (an epsilon residue here
        // would keep the read fractional forever and silently cost FUNC-02).
        if (depth < depthTarget)      depth = juce::jmin (depthTarget, depth + depthStep);
        else if (depth > depthTarget) depth = juce::jmax (depthTarget, depth - depthStep);

        if (depth == 0.0)
            return 0.0;

        return depth * total;
    }

private:
    struct Partial
    {
        double phase;
        double freq;
        double baseFreq;
        double amp;
        double ampTarget;
        double deviation;   // peak read-rate deviation contributed at depth 1
        bool   walks;       // flutter: frequency random-walks 7-55 Hz
    };

    double fs = 48000.0;

    double depth       = 0.0;
    double depthTarget = 0.0;
    double depthStep   = 1.0;

    int redrawT       = 0;
    int redrawSamples = 1;

    double ampCoeff  = 0.0;
    double freqCoeff = 0.0;

    double flutterFreqTarget = 19.0;

    Partial partials[3] { { 0.0, 0.73, 0.73, 1.0, 1.0, 0.010, false },
                          { 0.0, 2.31, 2.31, 1.0, 1.0, 0.005, false },
                          { 0.0, 19.0, 19.0, 1.0, 1.0, 0.005, true  } };
};
