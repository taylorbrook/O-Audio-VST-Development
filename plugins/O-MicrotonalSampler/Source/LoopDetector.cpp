/*
  ==============================================================================

    LoopDetector.cpp
    Microtonal Sample Engine - Loop region auto-detection (Phase 2.5)
    Ouaricon Audio
    Developer: Taylor Brook

    Algorithm (per RESEARCH RQ-2 + PLAN Task 29):
      1. Mixdown channel 0 (+ channel 1 / 2 if stereo) into a working buffer.
      2. RMS scan: 1024-sample window, stride 256, search range
         [N * 0.40, N - windowSize - 64]. Track argmin and the running mean.
      3. Variance gate (EC-7): if rms[minIdx] / meanRms > 0.7 → fall back to
         one-shot (no clearly quiet region — likely a transient like a kick).
      4. Zero-crossing snap for loopStart in [minIdx - 64, minIdx + 64], with
         falling-edge preference.
      5. Target loop length max(2048, sampleRate / 50). Scan
         [loopStart + targetLen ± 64] for SAME-direction zc → loopEnd.
         If no headroom for the 8-sample crossfade (loopEnd >= N - 16),
         shrink targetLen to N - loopStart - 64 and re-scan; if STILL no fit,
         return invalid.
      6. Defensive length guard: loopEnd - loopStart < 16 → invalid.

    Allocation lives on the loader thread (one std::vector per slot in
    `mixdown`). RT-safe by construction — never invoked from processBlock.

  ==============================================================================
*/

#include "LoopDetector.h"

#include <cmath>
#include <limits>
#include <vector>

namespace LoopDetector
{
namespace
{
    constexpr int    kWindowSize     = 1024;
    constexpr int    kStride         = 256;
    constexpr float  kVarianceRatio  = 0.7f;   // rms[min] / meanRms must be < this
    constexpr int    kZcSearchRadius = 64;
    constexpr int    kMinLoopLength  = 16;     // crossfade is 8; require 2× headroom
    constexpr int    kEndpointMargin = 64;     // searchEnd guard band
    constexpr int    kCrossfadeLen   = 8;      // matches kLoopXfadeLUT in voice
    constexpr double kSearchStartFrac = 0.40;

    // Mixdown to mono (sum / numChannels). One-time alloc on loader thread.
    std::vector<float> makeMixdown (const juce::AudioBuffer<float>& buf)
    {
        const int N    = buf.getNumSamples();
        const int chs  = buf.getNumChannels();
        std::vector<float> out (static_cast<size_t> (N), 0.0f);

        if (N <= 0 || chs <= 0)
            return out;

        const float invCh = 1.0f / static_cast<float> (chs);
        for (int ch = 0; ch < chs; ++ch)
        {
            const float* src = buf.getReadPointer (ch);
            for (int i = 0; i < N; ++i)
                out[static_cast<size_t> (i)] += src[i] * invCh;
        }
        return out;
    }

    // RMS over [start, start + kWindowSize). Caller guarantees bounds.
    float rmsAt (const float* x, int start)
    {
        double acc = 0.0;
        for (int i = 0; i < kWindowSize; ++i)
        {
            const double v = static_cast<double> (x[start + i]);
            acc += v * v;
        }
        return static_cast<float> (std::sqrt (acc / static_cast<double> (kWindowSize)));
    }

    // Find the first sign change in [lo, hi). If `preferFalling` is true,
    // prefer transitions where x[i-1] > 0 && x[i] <= 0; otherwise return the
    // first sign change of either polarity. Returns -1 if none found.
    int findZeroCrossing (const float* x, int lo, int hi, bool preferFalling)
    {
        if (lo < 1) lo = 1;
        if (hi <= lo) return -1;

        int firstAny = -1;
        for (int i = lo; i < hi; ++i)
        {
            const float prev = x[i - 1];
            const float curr = x[i];
            const bool falling = (prev > 0.0f && curr <= 0.0f);
            const bool rising  = (prev < 0.0f && curr >= 0.0f);

            if (preferFalling)
            {
                if (falling) return i;
                if (firstAny < 0 && rising) firstAny = i;
            }
            else
            {
                if (falling || rising) return i;
            }
        }
        return preferFalling ? firstAny : -1;
    }

    // Find a zero-crossing of the same polarity-direction as the one at
    // `referenceZc` within [lo, hi). Returns -1 if none found.
    int findMatchingDirectionZc (const float* x, int referenceZc, int lo, int hi)
    {
        if (referenceZc < 1) return -1;
        const bool refFalling = (x[referenceZc - 1] > 0.0f && x[referenceZc] <= 0.0f);

        if (lo < 1) lo = 1;
        if (hi <= lo) return -1;

        for (int i = lo; i < hi; ++i)
        {
            const float prev = x[i - 1];
            const float curr = x[i];
            const bool falling = (prev > 0.0f && curr <= 0.0f);
            const bool rising  = (prev < 0.0f && curr >= 0.0f);
            if (refFalling ? falling : rising)
                return i;
        }
        return -1;
    }
} // anonymous namespace

LoopRegion detectLoop (const juce::AudioBuffer<float>& buf, double sampleRate)
{
    LoopRegion fail; // valid = false, loopStart = loopEnd = 0

    const int N = buf.getNumSamples();
    if (N <= 0 || sampleRate <= 0.0)
        return fail;

    // Need enough material for a search window past the 40% mark, with
    // headroom for endpoint margin and crossfade.
    const int searchStart = static_cast<int> (static_cast<double> (N) * kSearchStartFrac);
    const int searchEnd   = N - kWindowSize - kEndpointMargin;
    if (searchEnd <= searchStart || searchStart < 0)
        return fail;

    const auto mix = makeMixdown (buf);
    const float* x = mix.data();

    // ---- 2. RMS scan with stride ----
    int   minIdx  = -1;
    float minRms  = std::numeric_limits<float>::infinity();
    double sumRms = 0.0;
    int    count  = 0;

    for (int i = searchStart; i <= searchEnd; i += kStride)
    {
        const float r = rmsAt (x, i);
        sumRms += static_cast<double> (r);
        ++count;
        if (r < minRms)
        {
            minRms = r;
            minIdx = i;
        }
    }

    if (count == 0 || minIdx < 0)
        return fail;

    const float meanRms = static_cast<float> (sumRms / static_cast<double> (count));

    // ---- 3. Variance gate (EC-7 — transient/percussive content) ----
    // meanRms == 0 implies silence; treat as invalid.
    if (meanRms <= 0.0f)
        return fail;
    if ((minRms / meanRms) > kVarianceRatio)
        return fail;

    // ---- 4. Zero-crossing snap for loopStart ----
    const int zcLo = juce::jmax (1,            minIdx - kZcSearchRadius);
    const int zcHi = juce::jmin (N - 1,        minIdx + kZcSearchRadius);
    int loopStart  = findZeroCrossing (x, zcLo, zcHi, /*preferFalling*/ true);
    if (loopStart < 0)
        loopStart = minIdx;

    // ---- 5. Target loop length, then matching-direction zc for loopEnd ----
    int targetLen = juce::jmax (2048, static_cast<int> (sampleRate / 50.0));

    auto scanLoopEnd = [&] (int targetLength) -> int
    {
        const int centre = loopStart + targetLength;
        const int lo     = juce::jmax (loopStart + 2, centre - kZcSearchRadius);
        const int hi     = juce::jmin (N - 1,         centre + kZcSearchRadius);
        if (hi <= lo)
            return -1;
        return findMatchingDirectionZc (x, loopStart, lo, hi);
    };

    int loopEnd = scanLoopEnd (targetLen);

    // Re-scan with shrunken target if the first attempt landed past the
    // end-of-buffer crossfade headroom or wasn't found at all.
    if (loopEnd < 0 || loopEnd >= N - kCrossfadeLen - kEndpointMargin / 8)
    {
        const int shrunken = N - loopStart - kEndpointMargin;
        if (shrunken > kMinLoopLength)
            loopEnd = scanLoopEnd (shrunken);
    }

    if (loopEnd < 0)
        return fail;

    // ---- 6. Defensive length guard ----
    if ((loopEnd - loopStart) < kMinLoopLength)
        return fail;

    // Final headroom check for the 8-sample crossfade — the renderer reads
    // up to (loopEnd - 1) so loopEnd must leave room for the cubic 4-tap
    // context (i+2). Caller's slot has N samples; require loopEnd <= N - 2.
    if (loopEnd > N - 2)
        return fail;

    LoopRegion ok;
    ok.loopStart = loopStart;
    ok.loopEnd   = loopEnd;
    ok.valid     = true;
    return ok;
}
} // namespace LoopDetector
