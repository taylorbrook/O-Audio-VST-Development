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

void DistortionProcessor::prepare (const juce::dsp::ProcessSpec& spec)
{
    oversampling.initProcessing (spec.maximumBlockSize);
    dryWetMixer.prepare (spec);

    // Tracks the oversampler automatically -- nothing here needs to know the
    // factor. The antialiased Fold adds a further half sample of group delay AT
    // THE OVERSAMPLED RATE (0.125 samples at base rate for 4x), which is NOT
    // included here and is left uncompensated as inaudible.
    dryWetMixer.setWetLatency (static_cast<float> (oversampling.getLatencyInSamples()));

    for (auto& u : adaaPrevU)
        u = 0.0;
}

void DistortionProcessor::reset()
{
    oversampling.reset();
    dryWetMixer.reset();

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
    dryWetMixer.setWetMixProportion (mix);
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
    dryWetMixer.pushDrySamples (block);

    auto osBlock = oversampling.processSamplesUp (block);
    applyDistortion (osBlock);
    oversampling.processSamplesDown (block);

    dryWetMixer.mixWetSamples (block);
}
