/*
   This file is part of O-Freeze, an Ouaricon Audio plugin.
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

    SpectralFreeze.cpp - see SpectralFreeze.h

    Capture (milestone spectral-diffusion-freeze, RESEARCH § Recommended Approach):
      |X_k| = sqrt(mean_N |X_k|^2)                        RMS average of N frames
      w_k H = k pi / 2 + princarg(dphi_k - k pi / 2)      IF from the newest two frames
      peaks: local max over +-2 bins, > -60 dB re max; regions end at the minimum
    Resynthesis, once per hop on the synth clock, per bin k (draws shared by L/R):
      phi_f += w_k H 2^(r_k cents / 1200) + beta pi (2u - 1)
      phi    = phi_f + lambda(beta) princarg(phi_peak + delta_k - phi_f),
               lambda = max(0, 1 - beta / 0.3)^2              soft identity lock
      d     <- a d + sqrt(1 - a^2) sqrt(3) (2u - 1)           unit-variance OU walk
      |Y_k|  = |X_k| 10^(1.5 dB * drift * d / 20)
      y      = IFFT(Y) * hann / 1.5 * makeup(beta) * trim, overlap-added

  ==============================================================================
*/

#include "SpectralFreeze.h"
#include "CounterRng.h"
#include <cmath>
#include <cstring>
#if JUCE_MAC
 #include <Accelerate/Accelerate.h>
#endif

namespace
{
    constexpr double kPi = juce::MathConstants<double>::pi;
    constexpr double kTwoPi = juce::MathConstants<double>::twoPi;
    constexpr float kPiF = juce::MathConstants<float>::pi;
    constexpr float kTwoPiF = juce::MathConstants<float>::twoPi;

    inline double princarg(double x) noexcept { return x - kTwoPi * std::round(x / kTwoPi); }
    inline float princargF(float x) noexcept { return x - kTwoPiF * std::round(x / kTwoPiF); }

    constexpr size_t sz(int i) noexcept { return static_cast<size_t>(i); }

    // out[k] = atan2(y[k], x[k]) over all kBins bins (vForce on macOS)
    inline void atan2Bins(float* out, const float* y, const float* x) noexcept
    {
       #if JUCE_MAC
        const int n = SpectralFreeze::kBins;
        vvatan2f(out, y, x, &n);
       #else
        for (int k = 0; k < SpectralFreeze::kBins; ++k)
            out[k] = std::atan2(y[k], x[k]);
       #endif
    }
}

SpectralFreeze::SpectralFreeze()
{
    // Everything the DSP touches is sized once here (the FFT object above allocates
    // in its constructor; prepare() only re-initialises).
    window.resize(sz(kSize));
    for (int i = 0; i < kSize; ++i)
        window[sz(i)] = static_cast<float>(0.5 - 0.5 * std::cos(kTwoPi * static_cast<double>(i) / static_cast<double>(kSize)));

    work.assign(sz(2 * kSize), 0.0f);
    powerRing.assign(sz(kMaxAvg * kMaxChannels * kBins), 0.0f);
    specRing.assign(sz(2 * kMaxChannels * 2 * kBins), 0.0f);
    mag.assign(sz(kMaxChannels * kBins), 0.0f);
    omegaH.assign(sz(kMaxChannels * kBins), 0.0);
    phase.assign(sz(kMaxChannels * kBins), 0.0f);
    delta.assign(sz(kMaxChannels * kBins), 0.0f);
    peakOf.assign(sz(kBins), 0);
    power.assign(sz(kBins), 0.0f);
    peaks.assign(sz(kBins), 0);
    detuneR.assign(sz(kBins), 0.0f);
    walkA.assign(sz(kBins), 0.0f);
    walkB.assign(sz(kBins), 0.0f);
    walk.assign(sz(kBins), 0.0f);
    gainK.assign(sz(kBins), 1.0f);
    noiseK.assign(sz(kBins), 0.0f);
    factorK.assign(sz(kBins), 1.0f);
    advWrapped.assign(sz(kMaxChannels * kBins), 0.0f);
    sinK.assign(sz(kBins), 0.0f);
    cosK.assign(sz(kBins), 0.0f);
    ola.assign(sz(kMaxChannels * kOlaSize), 0.0f);
}

void SpectralFreeze::prepare(double newSampleRate, int newNumChannels, Slots newSlots)
{
    sampleRate = newSampleRate;
    numChannels = juce::jlimit(1, kMaxChannels, newNumChannels);
    slots = newSlots;
    hopCounter = 0;
    engageCounter = 0;

    std::fill(powerRing.begin(), powerRing.end(), 0.0f);
    std::fill(specRing.begin(), specRing.end(), 0.0f);
    std::fill(ola.begin(), ola.end(), 0.0f);
    ringHead = 0;
    specLatest = 1;
    framesValid = 0;
    analysisCountdown = kHop;
    olaPos = 0;
    synthCountdown = 0;
    running = false;
    priming = false;
    onsetLength = juce::jmax(1, static_cast<int>(std::lround(kOnsetFadeSeconds * sampleRate)));
    onsetPos = onsetLength;
}

void SpectralFreeze::setSeed(uint64_t newSeed) noexcept
{
    seed = newSeed;
    hopCounter = 0;
    engageCounter = 0;
}

void SpectralFreeze::restartAnalysis() noexcept
{
    framesValid = 0;
    analysisCountdown = kHop;
}

float SpectralFreeze::uniform(uint64_t counter, uint32_t slot) const noexcept
{
    return CounterRng::uniform(seed, counter, slot);
}

float SpectralFreeze::makeupFor(float beta) noexcept
{
    // Incoherent frames lose 1/O = -6 dB under 4x OLA (RESEARCH F1); a tone's hop-to-hop
    // correlation under beta pi U(-1, 1) phase noise is sinc(beta). Half-compensation.
    const double x = kPi * static_cast<double>(beta);
    const double s = x < 1.0e-6 ? 1.0 : std::sin(x) / x;
    return static_cast<float>(1.0 / std::sqrt(1.0 - static_cast<double>(kMakeupAmount) * (1.0 - s * s)));
}

void SpectralFreeze::onWriteAdvanced(const juce::AudioBuffer<float>& ring, int writePosAfter) noexcept
{
    if (--analysisCountdown > 0)
        return;

    analysisCountdown = kHop;
    analyseFrame(ring, writePosAfter);
    ++rollingFrames;
}

void SpectralFreeze::analyseFrame(const juce::AudioBuffer<float>& ring, int endPos) noexcept
{
    const int len = ring.getNumSamples();
    const int start = ((endPos - kSize) % len + len) % len;
    const int first = juce::jmin(kSize, len - start);   // samples before the ring wraps

    const int powerSlot = ringHead;
    const int specSlot = 1 - specLatest;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* src = ring.getReadPointer(juce::jmin(ch, ring.getNumChannels() - 1));
        float* w = work.data();
        std::memcpy(w, src + start, sz(first) * sizeof(float));
        if (first < kSize)
            std::memcpy(w + first, src, sz(kSize - first) * sizeof(float));
        juce::FloatVectorOperations::multiply(w, window.data(), kSize);
        std::fill(w + kSize, w + 2 * kSize, 0.0f);

        fft.performRealOnlyForwardTransform(w, true);

        float* pw = powerRing.data() + sz((powerSlot * kMaxChannels + ch) * kBins);
        for (int k = 0; k < kBins; ++k)
            pw[k] = w[2 * k] * w[2 * k] + w[2 * k + 1] * w[2 * k + 1];

        std::memcpy(specRing.data() + sz((specSlot * kMaxChannels + ch) * 2 * kBins), w, sz(2 * kBins) * sizeof(float));
    }

    ringHead = (ringHead + 1) % kMaxAvg;
    specLatest = specSlot;
    framesValid = juce::jmin(framesValid + 1, kMaxAvg);
}

void SpectralFreeze::capture(const juce::AudioBuffer<float>& ring, int writePos, int avgFrames, const Controls& c) noexcept
{
    primeAvg = juce::jlimit(1, kMaxAvg, avgFrames);
    const int need = juce::jmax(primeAvg, 2);   // the IF estimate needs two frames

    // Fallback: too few rolling frames (engine just switched, or a mid-freeze switch,
    // where the write head is held): analyse them from the ring, ending at the write
    // head, as the first prime steps. One function, two callers.
    primeAnalysesLeft = 0;
    if (framesValid < need)
    {
        primeAnalysesLeft = need;
        ++fallbackCaptures;
    }
    ++captures;

    primeRing = &ring;
    primeWritePos = writePos;
    primeControls = c;
    primeStage = 0;
    primeCountdown = 1;   // the first step runs on this sample
    priming = true;
    running = true;
}

void SpectralFreeze::primeStep() noexcept
{
    if (primeAnalysesLeft > 0)
    {
        const int len = primeRing->getNumSamples();
        for (int i = 0; i < kFallbackPerStep && primeAnalysesLeft > 0; ++i)
        {
            const int j = --primeAnalysesLeft;   // need - 1 down to 0, oldest first
            analyseFrame(*primeRing, ((primeWritePos - j * kHop) % len + len) % len);
        }
        return;
    }

    const int stage = primeStage++;
    if (stage == 0)
        captureAverage();
    else if (stage == 1)
        capturePeaks();
    else if (stage == 2)
        captureDraws();
    else
    {
        // Prime the OLA: the frames that would have started 3, 2, 1 and 0 hops ago, so
        // the output is at its steady level on its first sample (an empty OLA ramps
        // over 3 hops).
        const int j = kPrimeStages - 1 - stage;
        if (j == 3)
        {
            std::fill(ola.begin(), ola.end(), 0.0f);
            olaPos = 0;
        }
        synthesiseFrame(-j * kHop, primeControls);
        if (j == 0)
        {
            synthCountdown = kHop;
            priming = false;
            onsetPos = 0;
        }
    }
}

void SpectralFreeze::captureAverage() noexcept
{
    // RMS magnitude average over the newest n frames, and phase / IF from the newest two
    const int n = primeAvg;
    const float invN = 1.0f / static_cast<float>(n);
    std::fill(power.begin(), power.end(), 0.0f);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* m = mag.data() + sz(ch * kBins);
        std::fill(m, m + kBins, 0.0f);
        for (int j = 0; j < n; ++j)
        {
            const int slot = ((ringHead - 1 - j) % kMaxAvg + kMaxAvg) % kMaxAvg;
            const float* pw = powerRing.data() + sz((slot * kMaxChannels + ch) * kBins);
            for (int k = 0; k < kBins; ++k)
                m[k] += pw[k];
        }
        for (int k = 0; k < kBins; ++k)
        {
            power[sz(k)] += m[k] * invN;
            m[k] = std::sqrt(m[k] * invN);
        }

        const float* x = specRing.data() + sz((specLatest * kMaxChannels + ch) * 2 * kBins);
        const float* y = specRing.data() + sz(((1 - specLatest) * kMaxChannels + ch) * 2 * kBins);
        float* ph = phase.data() + sz(ch * kBins);
        double* om = omegaH.data() + sz(ch * kBins);

        // phase of the newest frame, and dphi = arg(X_m conj(X_m-1)); sinK / cosK hold
        // the (im, re) pairs, noiseK receives dphi (all scratch at capture time)
        float* dphi = noiseK.data();
        for (int k = 0; k < kBins; ++k)
        {
            sinK[sz(k)] = x[2 * k + 1];
            cosK[sz(k)] = x[2 * k];
        }
        atan2Bins(ph, sinK.data(), cosK.data());
        for (int k = 0; k < kBins; ++k)
        {
            const float xr = x[2 * k], xi = x[2 * k + 1], yr = y[2 * k], yi = y[2 * k + 1];
            sinK[sz(k)] = xi * yr - xr * yi;
            cosK[sz(k)] = xr * yr + xi * yi;
        }
        atan2Bins(dphi, sinK.data(), cosK.data());
        for (int k = 0; k < kBins; ++k)
        {
            const double nominal = kPi * 0.5 * static_cast<double>(k);   // w_k H = 2 pi k H / W
            om[k] = nominal + princarg(static_cast<double>(dphi[k]) - nominal);
            advWrapped[sz(ch * kBins + k)] = static_cast<float>(princarg(om[k]));
        }

        dcValue[ch] = x[0] < 0.0f ? -m[0] : m[0];
        nyqValue[ch] = x[2 * (kBins - 1)] < 0.0f ? -m[kBins - 1] : m[kBins - 1];
    }
}

void SpectralFreeze::capturePeaks() noexcept
{
    // Peaks of the L+R power: local max over +-2 bins, above -60 dB (magnitude) re max.
    // Each bin locks to the nearest peak, the boundary being the minimum between two.
    float maxP = 0.0f;
    for (int k = 1; k < kBins - 1; ++k)
        maxP = juce::jmax(maxP, power[sz(k)]);
    const float floorP = maxP * 1.0e-6f;

    int numPeaks = 0;
    for (int k = 1; k < kBins - 1; ++k)
    {
        const float p = power[sz(k)];
        if (p <= floorP)
            continue;
        const bool left = p > power[sz(k - 1)] && (k < 2 || p > power[sz(k - 2)]);
        const bool right = p >= power[sz(k + 1)] && (k + 2 >= kBins || p >= power[sz(k + 2)]);
        if (left && right)
            peaks[sz(numPeaks++)] = k;
    }

    for (int k = 0; k < kBins; ++k)
        peakOf[sz(k)] = k;

    if (numPeaks > 0)
    {
        for (int k = 1; k < peaks[0]; ++k)
            peakOf[sz(k)] = peaks[0];

        for (int i = 0; i < numPeaks; ++i)
        {
            const int p = peaks[sz(i)];
            const int next = i + 1 < numPeaks ? peaks[sz(i + 1)] : kBins - 1;
            int boundary = next - 1;          // last bin of p's region
            if (i + 1 < numPeaks)
            {
                float minP = power[sz(p)];
                for (int k = p + 1; k < next; ++k)
                    if (power[sz(k)] < minP)
                    {
                        minP = power[sz(k)];
                        boundary = k;
                    }
            }
            for (int k = p + 1; k <= boundary; ++k)
                peakOf[sz(k)] = p;
            for (int k = boundary + 1; k < next; ++k)
                peakOf[sz(k)] = next;
        }
    }

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* ph = phase.data() + sz(ch * kBins);
        float* dl = delta.data() + sz(ch * kBins);
        for (int k = 0; k < kBins; ++k)
            dl[k] = ph[k] - ph[peakOf[sz(k)]];
    }
}

void SpectralFreeze::captureDraws() noexcept
{
    // Per-engage draws: detune factor r_k in [-1, 1), walk rate log-uniform 0.2 - 3 Hz.
    // One engage = 2 * kBins counters on the engage slot (detune, then rate).
    const uint64_t base = engageCounter++ * static_cast<uint64_t>(2 * kBins);
    for (int k = 0; k < kBins; ++k)
    {
        const uint64_t kk = static_cast<uint64_t>(k);
        detuneR[sz(k)] = uniform(base + kk, slots.engage) * 2.0f - 1.0f;
        const double rho = 0.2 * std::pow(15.0, static_cast<double>(uniform(base + static_cast<uint64_t>(kBins) + kk, slots.engage)));
        const double a = std::exp(-kTwoPi * rho * static_cast<double>(kHop) / sampleRate);
        walkA[sz(k)] = static_cast<float>(a);
        walkB[sz(k)] = static_cast<float>(std::sqrt((1.0 - a * a) * 3.0));
        walk[sz(k)] = 0.0f;
    }
}

void SpectralFreeze::synthesiseFrame(int offset, const Controls& c) noexcept
{
    // The whole per-hop update lives here, so amortising it (RESEARCH Alt 2) stays a
    // change to this function only.
    const uint64_t base = hopCounter++ * static_cast<uint64_t>(kBins);
    const float beta = juce::jlimit(0.0f, 1.0f, c.beta);
    const float noiseScale = beta * kPiF;
    const float l = juce::jmax(0.0f, 1.0f - beta / 0.3f);
    const float lambda = l * l;
    const float driftDb = juce::jlimit(0.0f, 1.0f, c.drift01) * 1.5f;
    const float driftLn = driftDb * (2.302585093f / 20.0f);
    const float detuneOct = c.detuneCents / 1200.0f;

    // Shared L/R per-bin factors. The walk always advances, so a live Drift move only
    // rescales a stationary walk (no jump).
    for (int k = 1; k < kBins - 1; ++k)
    {
        const uint64_t ctr = base + static_cast<uint64_t>(k);
        walk[sz(k)] = walkA[sz(k)] * walk[sz(k)] + walkB[sz(k)] * (uniform(ctr, slots.drift) * 2.0f - 1.0f);
        noiseK[sz(k)] = noiseScale > 0.0f ? noiseScale * (uniform(ctr, slots.phase) * 2.0f - 1.0f) : 0.0f;
        gainK[sz(k)] = driftLn > 0.0f ? std::exp(driftLn * walk[sz(k)]) : 1.0f;
        factorK[sz(k)] = detuneOct != 0.0f ? std::exp2(detuneR[sz(k)] * detuneOct) : 1.0f;
    }

    const float gain = makeupFor(beta) * kSpectralTrim / 1.5f;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* ph = phase.data() + sz(ch * kBins);
        const double* om = omegaH.data() + sz(ch * kBins);
        const float* m = mag.data() + sz(ch * kBins);
        const float* dl = delta.data() + sz(ch * kBins);

        // Free phase advance (peaks first: the lock below reads their new phase)
        if (detuneOct != 0.0f)
            for (int k = 1; k < kBins - 1; ++k)
            {
                const double adv = princarg(om[k] * static_cast<double>(factorK[sz(k)]));
                ph[k] = princargF(ph[k] + static_cast<float>(adv) + noiseK[sz(k)]);
            }
        else
        {
            const float* aw = advWrapped.data() + sz(ch * kBins);
            for (int k = 1; k < kBins - 1; ++k)
                ph[k] = princargF(ph[k] + aw[k] + noiseK[sz(k)]);
        }

        // Soft identity lock: lambda 1 at beta 0 (Laroche-Dolson), 0 from beta 0.3
        if (lambda > 0.0f)
            for (int k = 1; k < kBins - 1; ++k)
            {
                const int p = peakOf[sz(k)];
                if (p != k)
                    ph[k] = princargF(ph[k] + lambda * princargF(ph[p] + dl[k] - ph[k]));
            }

       #if JUCE_MAC
        const int nb = kBins;
        vvsincosf(sinK.data(), cosK.data(), ph, &nb);
       #else
        for (int k = 1; k < kBins - 1; ++k)
        {
            sinK[sz(k)] = std::sin(ph[k]);
            cosK[sz(k)] = std::cos(ph[k]);
        }
       #endif

        float* w = work.data();
        w[0] = dcValue[ch];
        w[1] = 0.0f;
        for (int k = 1; k < kBins - 1; ++k)
        {
            const float a = m[k] * gainK[sz(k)];
            w[2 * k] = a * cosK[sz(k)];
            w[2 * k + 1] = a * sinK[sz(k)];
        }
        w[2 * (kBins - 1)] = nyqValue[ch];
        w[2 * (kBins - 1) + 1] = 0.0f;
        std::fill(w + 2 * kBins, w + 2 * kSize, 0.0f);

        fft.performRealOnlyInverseTransform(w);

        // window * gain, then overlap-add in at most two contiguous runs
        juce::FloatVectorOperations::multiply(w, window.data(), kSize);
        const int i0 = juce::jmax(0, -offset);
        const int dst0 = (olaPos + offset + i0) & (kOlaSize - 1);
        const int count = kSize - i0;
        const int run = juce::jmin(count, kOlaSize - dst0);
        float* o = ola.data() + sz(ch * kOlaSize);
        juce::FloatVectorOperations::addWithMultiply(o + dst0, w + i0, gain, run);
        if (run < count)
            juce::FloatVectorOperations::addWithMultiply(o, w + i0 + run, gain, count - run);
    }
}

void SpectralFreeze::processSample(float* out, const Controls& c) noexcept
{
    if (priming)
    {
        if (--primeCountdown <= 0)
        {
            primeStep();
            primeCountdown = kPrimeSpacing;
        }
        if (priming)
        {
            for (int ch = 0; ch < kMaxChannels; ++ch)
                out[ch] = 0.0f;
            return;
        }
        // the last step: output starts on this sample
    }

    if (synthCountdown <= 0)
    {
        synthesiseFrame(0, c);
        synthCountdown = kHop;
    }

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float& v = ola[sz(ch * kOlaSize + olaPos)];
        out[ch] = v;
        v = 0.0f;
    }
    if (onsetPos < onsetLength)
    {
        const float g = static_cast<float>(++onsetPos) / static_cast<float>(onsetLength);
        for (int ch = 0; ch < numChannels; ++ch)
            out[ch] *= g;
    }
    for (int ch = numChannels; ch < kMaxChannels; ++ch)
        out[ch] = out[0];

    olaPos = (olaPos + 1) & (kOlaSize - 1);
    --synthCountdown;
}
