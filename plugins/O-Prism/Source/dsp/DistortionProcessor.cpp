/*
   This file is part of O-Prism, an Ouaricon Audio plugin.
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

    DistortionProcessor.cpp
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#include "DistortionProcessor.h"
#include "MathConstants.h"

DistortionProcessor::DistortionProcessor()
    /*  IN-09, v1.29.0: 2^2 = 4x, up from 2x, and the DECIMATION FILTER changed from
        filterHalfBandPolyphaseIIR to filterHalfBandFIREquiripple. Every figure
        below is measured against the real processor by
        tests/distortion_alias_check.cpp.

        The two filters fail in complementary places, which is the whole reason
        both knobs had to move:

          - Polyphase IIR at max quality has the DEEPER stopband but a WIDE
            transition band. Alias landing just above the base Nyquist comes back
            barely attenuated.
          - FIR equiripple has the NARROW transition but a shallower stopband, so
            it catches that near-Nyquist content and misses a little of what lands
            far above.

        Low fundamentals scatter their alias widely, mostly deep in the stopband,
        where IIR wins. High fundamentals pile it just above Nyquist, where FIR
        wins -- and by much more: at 4 kHz / drive 1.0, Fold measured -7.2 dB under
        IIR against -21.7 dB under FIR.

        Which is why the factor comes along. FIR at 2x alone fixed the top octave
        but REGRESSED the three saturator modes by 0.6 to 2.9 dB at 110 and 440 Hz,
        where IIR's deeper stopband had been doing the work. Raising to 4x pushes
        the far-out alias down enough to cover FIR's shallower stopband, and 4x FIR
        then beats the pre-fix path at every one of the 24 saturator measurements by
        6.3 to 13.9 dB, and at every Fold measurement by 9.9 to 19.3 dB. It is the
        only configuration tested that regresses nothing.

        The cost is reported latency: 3.1 -> 59.5 samples at 48 kHz (~1.2 ms), for
        every distortion type, not just Fold, and paid whenever the distortion is
        un-bypassed -- which is the DEFAULT, since distBypass defaults to false even
        though distMix defaults to 0.0. It is FIXED for the session: this is the
        plugin's only latency source and its latency IS reported to the host
        (PluginProcessor.cpp:765, 1151), so making either the filter or the factor
        follow a parameter would make reported latency move when the user changed
        distortion type. Neither is per-mode for that reason.

        Rejected, with numbers, so it is not re-tried: raising the factor alone.
        With the antialiasing below in place, Fold at 110 Hz / drive 1.0 measured
        2x -29.2 dB, 4x -27.3, 8x -25.9 -- the bigger factors slightly WORSE, since
        under IIR the residual is transition-band content that no factor reaches.
    */
    : oversampling (kNumChannels, 2,
                    juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true)
{
}

namespace
{
    // Below this the wet path sleeps -- the same threshold the processor's
    // per-FX mix gate used before v1.30.1.
    constexpr float kWetFloor = 0.001f;
}

void DistortionProcessor::prepare (const juce::dsp::ProcessSpec& spec)
{
    preparedBlockSize = static_cast<int> (spec.maximumBlockSize);
    oversampling.initProcessing (spec.maximumBlockSize);

    // The antialiased Fold adds a further half sample of group delay AT THE
    // OVERSAMPLED RATE (0.125 samples at base rate for 4x), which is NOT
    // included here and is left uncompensated as inaudible.
    const float exact = getLatencyInSamples();
    const float frac = exact - std::floor (exact);
    constexpr float firDelay = (kHalfFirTaps - 1) * 0.5f;   // 15.5

    // Only the half-sample case exists (4x steep FIR = 59.5); any other
    // fraction would need a different FIR, so it falls back to truncation.
    dryUsesHalfFir = std::abs (frac - 0.5f) < 1.0e-3f && exact >= firDelay;
    jassert (dryUsesHalfFir || frac < 1.0e-3f);
    const int dryIntegerDelay = dryUsesHalfFir ? static_cast<int> (std::lround (exact - firDelay))
                                               : static_cast<int> (exact);

    // Kaiser-windowed sinc centred on 15.5, normalised to unity DC gain.
    {
        const auto besselI0 = [] (double x)
        {
            double sum = 1.0, term = 1.0;
            for (int k = 1; k < 30; ++k) { term *= (x / (2 * k)) * (x / (2 * k)); sum += term; }
            return sum;
        };
        constexpr double beta = 6.0;
        const double centre = (kHalfFirTaps - 1) * 0.5;
        double sum = 0.0;
        for (int k = 0; k < kHalfFirTaps; ++k)
        {
            const double d = k - centre;               // never 0: centre is a half-integer
            const double r = d / centre;
            const double w = besselI0 (beta * std::sqrt (juce::jmax (0.0, 1.0 - r * r))) / besselI0 (beta);
            halfFirCoeffs[static_cast<size_t> (k)] = static_cast<float> (std::sin (kPi * d) / (kPi * d) * w);
            sum += halfFirCoeffs[static_cast<size_t> (k)];
        }
        for (auto& c : halfFirCoeffs) c = static_cast<float> (c / sum);
    }

    dryDelay.setMaximumDelayInSamples (dryIntegerDelay + 4);
    dryDelay.prepare ({ spec.sampleRate, spec.maximumBlockSize, static_cast<juce::uint32> (kNumChannels) });
    dryDelay.setDelay (static_cast<float> (dryIntegerDelay));
    dryBuffer.setSize (static_cast<int> (kNumChannels), preparedBlockSize, false, false, true);

    mixSmoothed.reset (spec.sampleRate, 0.05);
    reset();
}

void DistortionProcessor::reset()
{
    oversampling.reset();
    dryDelay.reset();

    for (auto& h : halfFirHistory) h.fill (0.0f);
    halfFirPos = 0;

    mixSmoothed.setCurrentAndTargetValue (0.0f);
    wetAsleep = true;
    wakeHoldRemaining = 0;

    for (auto& u : adaaPrevU)
        u = 0.0;
}

void DistortionProcessor::setType (int type)
{
    distType = type;
}

void DistortionProcessor::setDrive (float drive)
{
    driveAmount = drive;
}

void DistortionProcessor::setMix (float mix)
{
    targetMix = juce::jlimit (0.0f, 1.0f, mix);
}

/*  IN-09 -- the antialiased Fold.

    Soft Clip, Hard Clip and Tube are monotonic saturators: their harmonic
    amplitudes fall away quickly, and they measure -21 to -51 dB alias-to-signal
    here, which is ordinary for an oversampled saturator. Fold is `sin (pi * u)`,
    which is PERIODIC -- as drive grows the input traverses more and more whole
    cycles, so its harmonic order is unbounded. At drive 1.0 the pre-gain is 10x
    and the order reaches ~37 PER INPUT PARTIAL. Against a band-limited saw
    carrying tens of partials the product needs roughly 36x oversampling to
    contain, so raising the factor cannot be the answer. It measurably is not:
    with the antialiasing below in place, alias-to-signal at 110 Hz / drive 1.0
    is 2x -29.2 dB, 4x -27.3 dB, 8x -25.9 dB -- the bigger factors are slightly
    WORSE, because what remains is not content above Nyquist but content inside
    the decimator's transition band. All figures in this file come from
    tests/distortion_alias_check.cpp against the real processor.

    So Fold is antialiased directly, by first-order antiderivative antialiasing
    (Parker, Zavalishin & D'Angelo 2016). `sin (pi * u)` has the closed-form
    antiderivative F(u) = -cos (pi * u) / pi, so the difference quotient

        y[n] = (F(u[n]) - F(u[n-1])) / (u[n] - u[n-1])

    is exact -- no approximation of the shaper is involved. It is the average of
    f over the segment the input travelled during the sample, which is precisely
    the band-limiting the naive point evaluation omits. With the 4x FIR decimator
    chosen in the constructor it takes Fold from -13.6 dB to -23.6 dB at 441 Hz /
    drive 1.0, and from -5.2 dB to -24.5 dB at 4 kHz, where the old output was
    more alias than Fold.

    THE TRADE, stated plainly: ADAA is not timbre-neutral. It band-limits by
    averaging f over each sample's excursion, and that average attenuates genuine
    high harmonics along with the alias, so Fold is duller than it was -- most
    audibly at mid pitch and moderate drive, where the old output was not
    especially alias-ridden to begin with. Up in the top octave the change runs
    the other way: there the old output was predominantly alias, so removing it
    moves the sound TOWARDS a true Fold rather than away from it. Section [H] of
    the gate measures the size of this change at each test point. Exactly one
    factory preset selects this mode ("Fold Engine", drive 0.5, mix 0.25, filter
    at 900 Hz), so the factory bank is affected at the mild end; user patches on
    Fold will shift.

    As u[n] approaches u[n-1] the quotient is 0/0, and in floating point it loses
    significance well before it divides by zero -- the two cosines agree to more
    digits than the denominator retains. Below `kAdaaEpsilon` it falls back to the
    midpoint evaluation, the value the quotient converges to. Silence lands in
    that branch (du = 0) and yields exactly 0, so the guard is also what keeps a
    silent block from producing NaN.
*/
static constexpr double kAdaaEpsilon = 1.0e-7;

static inline double foldAntiderivative (double u) noexcept
{
    return -std::cos (u * kPi) / kPi;
}

void DistortionProcessor::applyDistortion (juce::dsp::AudioBlock<float>& block)
{
    float gain = 1.0f + driveAmount * 9.0f;

    // Bounds `adaaPrevU`. `oversampling` is built for kNumChannels, so it cannot
    // hand back a wider block than this.
    jassert (block.getNumChannels() <= kNumChannels);

    for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
    {
        auto* data = block.getChannelPointer (ch);
        auto numSamples = block.getNumSamples();

        double prevU = adaaPrevU[ch];

        for (size_t i = 0; i < numSamples; ++i)
        {
            float x = data[i] * gain;

            switch (distType)
            {
                case 0: // Soft Clip
                    data[i] = std::tanh (x);
                    break;
                case 1: // Hard Clip
                    data[i] = juce::jlimit (-1.0f, 1.0f, x);
                    break;
                case 2: // Tube (asymmetric)
                    if (x >= 0.0f)
                        data[i] = x / (1.0f + std::abs (x));
                    else
                        data[i] = std::tanh (x * 1.5f) / 1.5f;
                    break;
                case 3: // Fold -- antialiased, see the note above
                {
                    const double u  = static_cast<double> (x);
                    const double du = u - prevU;

                    data[i] = static_cast<float> (std::abs (du) > kAdaaEpsilon
                        ? (foldAntiderivative (u) - foldAntiderivative (prevU)) / du
                        : std::sin (0.5 * (u + prevU) * kPi));
                    break;
                }
            }

            // Updated for every mode, not only Fold: switching INTO Fold must not
            // difference against a sample left over from whenever Fold last ran.
            prevU = static_cast<double> (x);
        }

        adaaPrevU[ch] = prevU;
    }
}

void DistortionProcessor::process (juce::dsp::AudioBlock<float>& block)
{
    // v1.30.1: never hand the oversampler more than it was prepared for.
    const int total = static_cast<int> (block.getNumSamples());
    const int maxChunk = juce::jmax (1, preparedBlockSize);
    for (int start = 0; start < total; start += maxChunk)
    {
        const int n = juce::jmin (maxChunk, total - start);
        processChunk (block.getSubBlock (static_cast<size_t> (start), static_cast<size_t> (n)));
    }
}

float DistortionProcessor::delayDrySample (size_t ch, float x) noexcept
{
    // Integer part first, then the 15.5-sample FIR. Mirrored ring: the newest
    // sample is written at pos and pos+TAPS, so hist[pos .. pos+TAPS) is always
    // contiguous, newest first. The caller advances halfFirPos once per sample.
    dryDelay.pushSample (static_cast<int> (ch), x);
    x = dryDelay.popSample (static_cast<int> (ch));

    if (! dryUsesHalfFir)
        return x;

    auto& hist = halfFirHistory[ch];
    hist[static_cast<size_t> (halfFirPos)] = x;
    hist[static_cast<size_t> (halfFirPos + kHalfFirTaps)] = x;
    float acc = 0.0f;
    for (int k = 0; k < kHalfFirTaps; ++k)
        acc += halfFirCoeffs[static_cast<size_t> (k)] * hist[static_cast<size_t> (halfFirPos + k)];
    return acc;
}

void DistortionProcessor::processDryOnly (juce::dsp::AudioBlock<float> chunk)
{
    const size_t numCh = juce::jmin (chunk.getNumChannels(), kNumChannels);
    const int n = static_cast<int> (chunk.getNumSamples());
    const int startPos = halfFirPos;

    for (size_t ch = 0; ch < numCh; ++ch)
    {
        auto* data = chunk.getChannelPointer (ch);
        halfFirPos = startPos;
        for (int i = 0; i < n; ++i)
        {
            halfFirPos = (halfFirPos == 0 ? kHalfFirTaps : halfFirPos) - 1;
            data[i] = delayDrySample (ch, data[i]);
        }
    }
}

void DistortionProcessor::processChunk (juce::dsp::AudioBlock<float> chunk)
{
    if (wetAsleep)
    {
        if (targetMix <= kWetFloor)
        {
            processDryOnly (chunk);
            return;
        }

        // Wake. The oversampler was reset on the way to sleep, so it emits its
        // latency in zeros before the signal arrives: hold the mix at 0 across
        // that, then ramp.
        wetAsleep = false;
        wakeHoldRemaining = static_cast<int> (std::ceil (getLatencyInSamples()));
        mixSmoothed.setCurrentAndTargetValue (0.0f);
    }

    const size_t numCh = juce::jmin (chunk.getNumChannels(), kNumChannels);
    const int n = static_cast<int> (chunk.getNumSamples());

    for (size_t ch = 0; ch < numCh; ++ch)
        juce::FloatVectorOperations::copy (dryBuffer.getWritePointer (static_cast<int> (ch)),
                                           chunk.getChannelPointer (ch), n);

    auto osBlock = oversampling.processSamplesUp (chunk);
    applyDistortion (osBlock);
    oversampling.processSamplesDown (chunk);

    // Linear law (dry = 1 - m), one ramp shared by both channels.
    const int startPos = halfFirPos;
    const int holdAtStart = wakeHoldRemaining;
    const auto mixAtStart = mixSmoothed;

    for (size_t ch = 0; ch < numCh; ++ch)
    {
        const float* dry = dryBuffer.getReadPointer (static_cast<int> (ch));
        auto* wet = chunk.getChannelPointer (ch);
        halfFirPos = startPos;
        int hold = holdAtStart;
        auto ramp = mixAtStart;
        ramp.setTargetValue (targetMix);

        for (int i = 0; i < n; ++i)
        {
            halfFirPos = (halfFirPos == 0 ? kHalfFirTaps : halfFirPos) - 1;
            const float d = delayDrySample (ch, dry[i]);

            float m = 0.0f;
            if (hold > 0)
                --hold;
            else
                m = ramp.getNextValue();

            wet[i] = (1.0f - m) * d + m * wet[i];
        }

        if (ch + 1 == numCh)
        {
            wakeHoldRemaining = hold;
            mixSmoothed = ramp;
        }
    }

    // Sleep once the ramp has landed at ~0. Resetting here is what lets the
    // wake path assume a zeroed oversampler.
    if (wakeHoldRemaining == 0 && targetMix <= kWetFloor && ! mixSmoothed.isSmoothing())
    {
        oversampling.reset();
        for (auto& u : adaaPrevU)
            u = 0.0;
        mixSmoothed.setCurrentAndTargetValue (0.0f);
        wetAsleep = true;
    }
}
