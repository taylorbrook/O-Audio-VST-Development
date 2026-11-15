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

    O-ParametricEQ - spectrum analyzer, message side (Stage 2, Phase 2.6;
    PLAN-2.6 P3; RESEARCH-2.6 Q2 "message side"; ARCHITECTURE "Analyzer" as
    amended)

    Reads the processor's two taps (eq::SpectrumTap, pre and post) and turns
    them into two arrays of 256 display points in dB. Message thread (the
    editor's timer in Stage 3; the render harness's own thread in Stage 2).
    Nothing here runs on the audio thread; the processor only writes the
    rings, and only while a consumer is registered and analyzerMode has the
    tap's bit (PLAN-2.6 P11). This class reads; the mode gating is the
    processor's.

    Consumer. The constructor registers this analyzer with the processor
    (setAnalyzerConsumer (true)), the destructor unregisters it. With no
    consumer the audio thread writes nothing.

    Per process (dtSeconds), in this order:

      0. prepare when needed: on the first call, and whenever the
         processor's preparedRate or either tap's generation differs from
         what this analyzer was prepared for. The rate is loaded before the
         generations: the processor bumps the generations first and
         publishes the rate after them (release), so a reader that sees the
         new rate sees the new generations too. Allocation happens here, on
         this thread.
           FFT size N = 4096 (fs <= 48 kHz), 8192 (<= 96 kHz), 16384 (above,
           also above 192 kHz); juce::dsp::FFT of order log2 (4 N) = 14 / 15
           / 16; per tap a history of N floats; one scratch of 2 x 4 N floats;
           the periodic Hann window w[i] = 0.5 - 0.5 cos (2 pi i / N) and
           its sum, in double. Before the processor's first prepare
           (preparedRate 0) the analyzer sizes itself for 48 kHz.
      1. drain each tap completely (maxN = SpectrumTap::capacity, every
         time: a partial drain can see the drop counter move one drain
         before the gap it counts arrives; wave-1 notes item 2) into its
         history, newest last.
           - a drain whose generation moved: the drained samples are
             discarded and the history's valid count cleared (the next call
             re-prepares);
           - a drain whose drop counter moved: the drained samples are
             discarded and the valid count cleared, and so are the samples of
             the NEXT drain (a drop counted between the drain's writeIndex
             load and its drop-counter load belongs to samples that arrive in
             the next drain). No analysis window straddles a gap.
      2. per tap that received samples in this call and holds at least N
         valid samples since the last clear: the newest N samples x w,
         zero-padded to 4 N, performFrequencyOnlyForwardTransform on the
         scratch (it works in place), per bin 20 log10 (|X| 2 / sum (w)),
         floor -200 dB. A tap that received nothing computes no frame.
      3. ballistics per bin: display = max (frame, held - 20 dt) with a
         frame, held - 20 dt without one (floor -200): instant attack, a
         release of 20 dB/s that keeps running when no data arrives, so the
         display falls when the host stops calling processBlock. dt is passed
         in, not read from a clock, so a render is deterministic (Stage 3
         passes the timer's measured interval).
      4. 256 display points over 10 Hz - 30 kHz. Point p spans
         [f_p, f_{p+1}), f_p = 10 x 3000^(p / 256). Its value is the largest
         held bin whose frequency (k fs / 4 N) lies in the span; a span that
         holds no bin reads the dB interpolation (linear in dB, linear in
         frequency) of the two bins around the span's centre
         sqrt (f_p f_{p+1}); a span that starts at or above fs / 2 reads the
         floor (the page draws it empty).

    Calibration (tap_bench `analyzer`; -12 dBFS sine on both channels, the
    largest display value, worst of 8 phases):

      window   pad   1 kHz at 44.1 / 96 / 192 kHz   300 tones 20 Hz - 20 kHz, 5 rates
      Hann     1x    -12.08 / -12.63 / -12.63       -13.42 ... -12.00
      Hann     2x    -12.08 / -12.16 / -12.16       -12.38 ... -12.00
      Hann     4x    -12.08 / -12.04 / -12.04       -12.11 ... -11.99   (this class)

    At 96 and 192 kHz the bins are 11.72 Hz apart and 1 kHz sits a third of
    a bin off-centre: Hann at 1x loses 0.63 dB to scalloping, outside the
    roadmap's +-0.5. The 4x pad interpolates the spectrum so the largest bin
    is never more than 1/8 bin from the peak (<= 0.11 dB anywhere) and leaves
    the resolution unchanged.

    Why Hann and not Blackman-Harris. Blackman-Harris 4 at 4x reads within
    0.05 dB, but its main lobe is about 36 % wider (32 Hz against 23 Hz at
    -6 dB): the display would smear low partials for an accuracy the
    roadmap does not need. A flat-top window reads exactly at 1x but its
    lobe meets the negative-frequency image at low frequencies (-10.77 dB on
    the sweep).

    The interpolation rule matters below about 90 Hz: the display points are
    narrower than a bin there even at 4x (the point at 10 Hz spans 0.32 Hz;
    bins are 2.9 Hz apart at 44.1 kHz), so most low spans hold no bin.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

class OuariconParametricEQAudioProcessor;

namespace eq
{
class SpectrumTap;
}

class SpectrumAnalyzer
{
public:
    static constexpr int numPoints = 256;
    static constexpr float floorDb = -200.0f;
    static constexpr double releaseDbPerSecond = 20.0;
    static constexpr double minDisplayHz = 10.0;
    static constexpr double maxDisplayHz = 30000.0;
    static constexpr int zeroPadFactor = 4;

    /** Registers as the processor's analyzer consumer. */
    explicit SpectrumAnalyzer (OuariconParametricEQAudioProcessor&);

    /** Unregisters. */
    ~SpectrumAnalyzer();

    /** One analyzer tick: see the file comment. dtSeconds is the time since
        the previous call (a negative or non-finite value is read as 0). */
    void process (double dtSeconds);

    /** The display points of the last process call, dB, numPoints each. */
    const float* getPre() const noexcept            { return taps[0].points.data(); }
    const float* getPost() const noexcept           { return taps[1].points.data(); }

    //==========================================================================
    // Read only, for the render harness
    //==========================================================================

    /** N for a rate: 4096 / 8192 / 16384 (fs <= 48 / <= 96 kHz / above). */
    static int fftSizeForRate (double sampleRateHz) noexcept;

    /** f_p = 10 x 3000^(p / 256), p = 0 ... 256. */
    static double pointFrequency (int p) noexcept;

    /** The window length N in use (0 before the first process call). */
    int getFftSize() const noexcept                 { return fftSize; }

    /** The rate the analyzer is prepared for (48000 before the processor's
        first prepare), and whether that came from the processor. */
    double getPreparedRate() const noexcept         { return preparedRate; }
    bool isRateKnown() const noexcept               { return rateKnown; }

    /** Number of prepares since construction. */
    int getPrepareCount() const noexcept            { return prepareCount; }

    /** Per tap (0 pre, 1 post). */
    std::uint32_t getGenerationSeen (int tap) const noexcept    { return tapAt (tap).generation; }
    std::uint32_t getDropsSeen (int tap) const noexcept         { return tapAt (tap).drops; }
    int getValidCount (int tap) const noexcept                  { return tapAt (tap).valid; }
    std::uint64_t getFramesComputed (int tap) const noexcept    { return tapAt (tap).frames; }
    std::uint64_t getSamplesDrained (int tap) const noexcept    { return tapAt (tap).drained; }
    std::uint64_t getSamplesDiscarded (int tap) const noexcept  { return tapAt (tap).discarded; }

    /** The held per-bin values (dB), 2 N + 1 bins of spacing fs / 4 N. */
    const std::vector<float>& getHeldBins (int tap) const noexcept { return tapAt (tap).held; }

private:
    struct TapState
    {
        eq::SpectrumTap* tap = nullptr;
        std::vector<float> history;     // N, newest last
        std::vector<float> held;        // 2 N + 1 bins, dB
        std::array<float, numPoints> points {};
        int valid = 0;                  // samples of history accumulated since the last clear
        bool skipNext = false;          // discard the next drain (a drop seen in this one)
        std::uint32_t generation = 0;
        std::uint32_t drops = 0;
        std::uint64_t frames = 0;
        std::uint64_t drained = 0;
        std::uint64_t discarded = 0;
    };

    const TapState& tapAt (int index) const noexcept { return taps[(index == 1) ? 1 : 0]; }

    void prepare (double sampleRateHz, bool known);

    /** Step 1; returns the number of samples appended to the history. */
    int drainTap (TapState&);

    /** Step 2 into `frameDb` (2 N + 1 bins). */
    void computeFrame (const TapState&);

    void reducePoints (TapState&) const;

    OuariconParametricEQAudioProcessor& processor;

    std::array<TapState, 2> taps;

    bool prepared = false;
    bool rateKnown = false;
    double preparedRate = 0.0;
    int prepareCount = 0;
    int fftSize = 0;
    int numBins = 0;

    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<double> window;
    double windowSum = 1.0;
    std::vector<float> scratch;         // 2 x 4 N
    std::vector<float> frameDb;         // 2 N + 1
    std::vector<float> drainBuffer;     // SpectrumTap::capacity

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumAnalyzer)
};
