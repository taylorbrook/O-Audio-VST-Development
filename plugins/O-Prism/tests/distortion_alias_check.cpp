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

    distortion_alias_check.cpp — O-Prism v1.29.0 distortion aliasing gate (IN-09).

    Before this gate the distortion stage had NO automated coverage of any kind:
    seven gates and not one of them drove DistortionProcessor.

    The defect. Soft Clip, Hard Clip and Tube are monotonic saturators whose
    harmonic amplitudes fall away quickly. Fold is `sin (pi * u)`, which is
    PERIODIC — as drive grows the input traverses whole cycles of it, so its
    harmonic order is unbounded. At drive 1.0 the pre-gain is 10x and the order
    reaches ~37 per input partial, so against a band-limited saw carrying tens of
    partials the product is far beyond what any affordable oversampling factor can
    contain. Measured at 441 Hz / drive 1.0 during the investigation: 2x =
    -17.2 dB alias-to-signal, 4x = -21.7, 8x = -23.2. That is why the review's
    prescribed fix — raise the factor — is not merely non-PATCH-shaped but
    INEFFECTIVE, and it is recorded here so it does not get re-attempted.

    The fix. Fold is antialiased directly, by first-order antiderivative
    antialiasing, on top of a factor raised 2x -> 4x. See the long note above
    `applyDistortion` in DistortionProcessor.cpp for the derivation and for the
    timbre trade this buys.

    The observable. Alias-to-signal ratio: drive a band-limited saw whose partials
    all sit below Nyquist, then measure how much energy comes back at frequencies
    that are NOT integer multiples of the fundamental. A memoryless shaper can only
    produce harmonics, so every off-harmonic component is alias by construction.
    The fundamental is chosen off-bin on purpose — with f0 an exact bin multiple,
    aliases of harmonic k fold back onto other harmonics of f0 and become
    invisible, and the gate would pass on any build at all.

    What this gate asserts:

      [A] Sanity — every mode renders finite and non-silent at every test point,
          and the band-limited excitation is itself clean (its own alias floor is
          far below anything the shapers produce). Without [A] the ratios below
          could be measuring the input.

      [B] THE FINDING. Fold's alias-to-signal must beat the pre-fix path by at
          least kMinImprovementDb at every pitch and drive. The pre-fix path —
          naive `sin (pi * u)` decimated by polyphase IIR at 2x, exactly what
          shipped through v1.28.1 — is reproduced inside this gate from its own
          Oversampling instance, so [B] carries its own NEGATIVE CONTROL and
          nothing has to be reverted in the source for the gate to be falsifiable.
          See kMinImprovementDb for why this is not stated as parity with the
          sibling modes.

      [C] Non-vacuity of that comparison. The replica must reproduce the code it
          mirrors and not something else: driven at the SHIPPED configuration (ADAA
          on, FIR at 2x) it must match the real processor bit for bit. If the
          replica has drifted, [B] is measuring a straw man, and [C] is what
          catches that.

      [D] The other three modes take no ADAA. Each must be BIT-IDENTICAL to its
          naive arithmetic reproduced in-gate at the shipped factor and filter, so
          the ADAA branch is proven to be reached only by case 3. Note what this
          does NOT claim: the decimator change moves all four modes, so Soft Clip,
          Hard Clip and Tube do not render as they did in v1.28.1 — [D] asserts
          that what changed for them is the filter and not the shaper.

      [E] The ill-conditioned branch is safe. As u[n] -> u[n-1] the ADAA quotient
          is 0/0, and below kAdaaEpsilon DistortionProcessor falls back to the
          midpoint. Silence (du == 0 every sample), DC, a hard step and a
          full-scale square must all render finite — this is the NaN trap, and it
          is armed: the same inputs through a fallback-free quotient produce
          non-finite output, which [E] also demonstrates.

      [F] State hygiene. reset() must clear the ADAA history, so a fresh render
          after reset is identical to the first one. A stale previous sample would
          put one wrong sample at the head of every render.

      [G] Diagnostic sweep over (factor, decimation filter), printing only. This is
          the evidence the shipped configuration was chosen from, kept so the choice
          can be re-derived rather than taken on trust.

      [H] Diagnostic, printing only: how far Fold's PARTIAL spectrum moves relative
          to the pre-fix path — the audible change to an existing Fold patch, as
          distinct from the alias removal that motivates it.

      [I] NON-REGRESSION for the three saturators. The decimator change is not
          Fold-scoped: it moves Soft Clip, Hard Clip and Tube as well. [D] proves
          only that the ADAA branch does not reach them, which is a claim about the
          shaper, not about the result. [I] is the claim about the result — none
          of the three may alias MORE than it did on the pre-fix path.

      [J] Diagnostic, printing only: what the new configuration costs in CPU per
          block, against the pre-fix one, per mode.

    Usage:  O-Prism-distortion-alias-check
    Exit code = number of failed checks.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "dsp/DistortionProcessor.h"
#include "dsp/MathConstants.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>

namespace
{
constexpr double kSampleRate = 48000.0;
constexpr int    kBlockSize  = 512;

// 2^15 = 32768 analysed samples: 1.46 Hz bins at 48 kHz, so a Blackman main
// lobe is ~9 Hz and the guard band below is a small fraction of the gap between
// partials even at the lowest fundamental tested.
constexpr int    kFftOrder = 15;
constexpr int    kFftSize  = 1 << kFftOrder;

// Blocks discarded before analysis, to clear the oversampler's and the dry/wet
// mixer's start-up transient out of the measured window.
constexpr int    kWarmupBlocks = 8;

// Bins masked either side of each expected partial. The Blackman main lobe is
// ~6 bins wide; 14 leaves generous room for skirt without swallowing the gaps.
constexpr int    kPartialGuardBins = 14;

/*  The acceptance criterion, and why it is NOT parity with the sibling modes.

    Parity was the first criterion tried and it is the wrong one. Fold and the
    three saturators are different nonlinearities: `tanh` is intrinsically gentle
    and `sin (pi * u)` intrinsically is not, so demanding that they alias equally
    demands something the arithmetic does not grant. Measured, the saturators sit
    at -29 to -41 dB and an antialiased Fold at -9 to -38 dB — close to them below
    1 kHz, still well behind in the top octave, and no decimator closes that gap.

    So the bound is IMPROVEMENT over the pre-fix path, measured against that path
    reproduced inside this gate. It is falsifiable (revert the fix and it fails),
    it cannot pass vacuously, and it states the actual claim: Fold aliases
    substantially less than it did at every pitch and drive tested. Parity with the
    saturators is still printed, as context rather than as a bound.

    8 dB is the floor; the smallest margin measured at the shipped configuration is
    9.9 dB, at 441 Hz / drive 1.0.
*/
constexpr double kMinImprovementDb = 8.0;

/*  The three saturator modes take no ADAA, but the decimator change moves all four
    modes, so "Fold improved" is only half the claim -- [I] pins the other half, and
    it is asserted with ZERO tolerance: every one of the 24 saturator measurements
    must be at least as good as the pre-fix path, not merely close to it.

    Zero is affordable because the shipped configuration was CHOSEN to make it so.
    FIR at 2x -- the obvious cheaper answer, and the one this gate rejected -- came
    in 0.6 to 2.9 dB WORSE at 110 and 440 Hz, because IIR's deeper stopband had been
    carrying the low fundamentals. Raising the factor to 4x recovers that and more:
    the measured margin is 6.3 to 13.9 dB of improvement. A future change that eats
    6 dB of it should fail here.
*/
constexpr double kSaturatorRegressionToleranceDb = 0.0;

int failures = 0;

void check (bool condition, const juce::String& what)
{
    if (condition)
        std::cout << "  ok    " << what << "\n";
    else
    {
        std::cout << "  FAIL  " << what << "\n";
        ++failures;
    }
}

juce::String db (double v, int places = 1) { return juce::String (v, places); }

enum class Mode { SoftClip = 0, HardClip = 1, Tube = 2, Fold = 3 };

juce::String modeName (Mode m)
{
    switch (m)
    {
        case Mode::SoftClip: return "Soft Clip";
        case Mode::HardClip: return "Hard Clip";
        case Mode::Tube:     return "Tube";
        case Mode::Fold:     return "Fold";
    }
    return "?";
}

//==============================================================================
// Excitation: a saw built additively from partials strictly below Nyquist, so
// the INPUT contributes no alias of its own. Deterministic — no RNG anywhere in
// this gate.
std::vector<float> bandLimitedSaw (double f0, int numSamples, double rate)
{
    const int kMax = static_cast<int> (std::floor ((rate * 0.5) / f0));

    std::vector<float> out (static_cast<size_t> (numSamples), 0.0f);
    double peak = 0.0;

    for (int i = 0; i < numSamples; ++i)
    {
        double acc = 0.0;
        for (int k = 1; k <= kMax; ++k)
            acc += std::sin (2.0 * kPi * k * f0 * i / rate) / k;

        out[static_cast<size_t> (i)] = static_cast<float> (acc);
        peak = std::max (peak, std::abs (acc));
    }

    if (peak > 0.0)
        for (auto& s : out)
            s = static_cast<float> (s / peak);

    return out;
}

//==============================================================================
/** Alias-to-signal ratio in dB: energy away from integer multiples of f0,
    against energy at them. Lower is better; 0 dB means the alias is as loud as
    the signal.
*/
double aliasToSignalDb (const std::vector<float>& signal, double f0, double rate)
{
    jassert (static_cast<int> (signal.size()) >= kFftSize);

    std::vector<float> fftData (static_cast<size_t> (2 * kFftSize), 0.0f);

    juce::dsp::WindowingFunction<float> window (static_cast<size_t> (kFftSize),
                                                juce::dsp::WindowingFunction<float>::blackman);

    std::copy (signal.begin(), signal.begin() + kFftSize, fftData.begin());
    window.multiplyWithWindowingTable (fftData.data(), static_cast<size_t> (kFftSize));

    juce::dsp::FFT fft (kFftOrder);
    fft.performFrequencyOnlyForwardTransform (fftData.data());

    const int    numBins  = kFftSize / 2;
    const double binWidth = rate / kFftSize;

    std::vector<bool> isPartial (static_cast<size_t> (numBins), false);

    for (int k = 1; k * f0 < rate * 0.5; ++k)
    {
        const int centre = static_cast<int> (std::llround (k * f0 / binWidth));

        for (int b = centre - kPartialGuardBins; b <= centre + kPartialGuardBins; ++b)
            if (b >= 0 && b < numBins)
                isPartial[static_cast<size_t> (b)] = true;
    }

    // DC and the lowest few bins carry windowing leakage and any DC offset the
    // asymmetric Tube shaper introduces; neither is alias.
    const int kLowSkip = 10;

    double partialPower = 0.0;
    double aliasPower   = 0.0;

    for (int b = kLowSkip; b < numBins; ++b)
    {
        const double p = static_cast<double> (fftData[static_cast<size_t> (b)])
                       * static_cast<double> (fftData[static_cast<size_t> (b)]);

        if (isPartial[static_cast<size_t> (b)])
            partialPower += p;
        else
            aliasPower += p;
    }

    if (partialPower <= 0.0)
        return std::numeric_limits<double>::infinity();

    return 10.0 * std::log10 (aliasPower / partialPower + 1.0e-30);
}

//==============================================================================
/** Deviation between two signals' PARTIAL magnitudes, in dB relative to the
    reference's partial energy. This is the audible timbre change — it ignores
    the off-harmonic bins entirely, so alias removal does not inflate it.
*/
double partialDeviationDb (const std::vector<float>& sig,
                           const std::vector<float>& ref, double f0, double rate)
{
    auto mags = [&] (const std::vector<float>& in)
    {
        std::vector<float> fftData (static_cast<size_t> (2 * kFftSize), 0.0f);
        juce::dsp::WindowingFunction<float> window (static_cast<size_t> (kFftSize),
                                                    juce::dsp::WindowingFunction<float>::blackman);
        std::copy (in.begin(), in.begin() + kFftSize, fftData.begin());
        window.multiplyWithWindowingTable (fftData.data(), static_cast<size_t> (kFftSize));
        juce::dsp::FFT fft (kFftOrder);
        fft.performFrequencyOnlyForwardTransform (fftData.data());
        return fftData;
    };

    const auto a = mags (sig);
    const auto b = mags (ref);

    const int    numBins  = kFftSize / 2;
    const double binWidth = rate / kFftSize;

    double num = 0.0, den = 0.0;

    for (int k = 1; k * f0 < rate * 0.5; ++k)
    {
        const int centre = static_cast<int> (std::llround (k * f0 / binWidth));

        // Peak within the guard band, so a fractional-bin partial is not
        // penalised for straddling two bins.
        double pa = 0.0, pb = 0.0;

        for (int bn = centre - kPartialGuardBins; bn <= centre + kPartialGuardBins; ++bn)
            if (bn >= 0 && bn < numBins)
            {
                pa = std::max (pa, static_cast<double> (a[static_cast<size_t> (bn)]));
                pb = std::max (pb, static_cast<double> (b[static_cast<size_t> (bn)]));
            }

        num += (pa - pb) * (pa - pb);
        den += pb * pb;
    }

    return 10.0 * std::log10 (num / (den + 1.0e-30) + 1.0e-30);
}

//==============================================================================
/** Renders the excitation through the REAL DistortionProcessor, fully wet. */
struct Rendered
{
    std::vector<float> audio;
    bool  finite = true;
    float peak   = 0.0f;
};

Rendered renderThroughPlugin (Mode mode, float drive, double f0,
                              int analysisLength = kFftSize)
{
    const int totalBlocks = kWarmupBlocks + (analysisLength + kBlockSize - 1) / kBlockSize + 1;
    const int totalLength = totalBlocks * kBlockSize;

    const auto excitation = bandLimitedSaw (f0, totalLength, kSampleRate);

    DistortionProcessor dist;
    dist.prepare ({ kSampleRate, static_cast<juce::uint32> (kBlockSize), 2 });
    dist.setType (static_cast<int> (mode));
    dist.setDrive (drive);
    dist.setMix (1.0f); // fully wet: measure the shaper, not the blend

    Rendered r;
    r.audio.reserve (static_cast<size_t> (totalLength));

    juce::AudioBuffer<float> buf (2, kBlockSize);

    for (int b = 0; b < totalBlocks; ++b)
    {
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < kBlockSize; ++i)
                buf.setSample (ch, i, excitation[static_cast<size_t> (b * kBlockSize + i)]);

        juce::dsp::AudioBlock<float> block (buf);
        dist.process (block);

        for (int i = 0; i < kBlockSize; ++i)
        {
            const float s = buf.getSample (0, i);

            if (! std::isfinite (s))
                r.finite = false;

            r.peak = juce::jmax (r.peak, std::abs (s));

            if (b >= kWarmupBlocks)
                r.audio.push_back (s);
        }
    }

    return r;
}

//==============================================================================
/** The pre-fix arithmetic, reproduced from its own Oversampling instance so the
    gate is falsifiable without touching the source. `factor` is the exponent:
    1 gives the 2x that shipped through v1.28.1, 2 the 4x that ships now.
    `useAdaa == false` is the naive point evaluation.
*/
Rendered renderReplica (Mode mode, float drive, double f0, size_t factor,
                        bool useAdaa, bool withEpsilonGuard = true,
                        juce::dsp::Oversampling<float>::FilterType filterType
                            = juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR)
{
    const int totalBlocks = kWarmupBlocks + (kFftSize + kBlockSize - 1) / kBlockSize + 1;
    const int totalLength = totalBlocks * kBlockSize;

    const auto excitation = bandLimitedSaw (f0, totalLength, kSampleRate);

    juce::dsp::Oversampling<float> os (2, factor, filterType, true);
    os.initProcessing (static_cast<size_t> (kBlockSize));

    const float gain = 1.0f + drive * 9.0f;
    double prevU[2] { 0.0, 0.0 };

    Rendered r;
    r.audio.reserve (static_cast<size_t> (totalLength));

    juce::AudioBuffer<float> buf (2, kBlockSize);

    for (int b = 0; b < totalBlocks; ++b)
    {
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < kBlockSize; ++i)
                buf.setSample (ch, i, excitation[static_cast<size_t> (b * kBlockSize + i)]);

        juce::dsp::AudioBlock<float> block (buf);
        auto osBlock = os.processSamplesUp (block);

        for (size_t ch = 0; ch < osBlock.getNumChannels(); ++ch)
        {
            auto* data = osBlock.getChannelPointer (ch);
            double p = prevU[ch];

            for (size_t i = 0; i < osBlock.getNumSamples(); ++i)
            {
                const float x = data[i] * gain;

                switch (mode)
                {
                    case Mode::SoftClip: data[i] = std::tanh (x); break;
                    case Mode::HardClip: data[i] = juce::jlimit (-1.0f, 1.0f, x); break;
                    case Mode::Tube:
                        data[i] = x >= 0.0f ? x / (1.0f + std::abs (x))
                                            : std::tanh (x * 1.5f) / 1.5f;
                        break;
                    case Mode::Fold:
                        if (! useAdaa)
                        {
                            data[i] = static_cast<float> (std::sin (x * kPi));
                        }
                        else
                        {
                            const double u  = static_cast<double> (x);
                            const double du = u - p;
                            const auto   F  = [] (double v) { return -std::cos (v * kPi) / kPi; };

                            data[i] = static_cast<float> (
                                (! withEpsilonGuard || std::abs (du) > 1.0e-7)
                                    ? (F (u) - F (p)) / du
                                    : std::sin (0.5 * (u + p) * kPi));
                        }
                        break;
                }

                p = static_cast<double> (x);
            }

            prevU[ch] = p;
        }

        os.processSamplesDown (block);

        for (int i = 0; i < kBlockSize; ++i)
        {
            const float s = buf.getSample (0, i);

            if (! std::isfinite (s))
                r.finite = false;

            r.peak = juce::jmax (r.peak, std::abs (s));

            if (b >= kWarmupBlocks)
                r.audio.push_back (s);
        }
    }

    return r;
}

//==============================================================================
/** Feeds an arbitrary waveform through the real processor. Used by [E] for the
    degenerate inputs that exercise the ill-conditioned ADAA branch.
*/
Rendered renderWaveform (Mode mode, float drive, const std::vector<float>& wave)
{
    DistortionProcessor dist;
    dist.prepare ({ kSampleRate, static_cast<juce::uint32> (kBlockSize), 2 });
    dist.setType (static_cast<int> (mode));
    dist.setDrive (drive);
    dist.setMix (1.0f);

    Rendered r;
    juce::AudioBuffer<float> buf (2, kBlockSize);

    const int numBlocks = static_cast<int> (wave.size()) / kBlockSize;

    for (int b = 0; b < numBlocks; ++b)
    {
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < kBlockSize; ++i)
                buf.setSample (ch, i, wave[static_cast<size_t> (b * kBlockSize + i)]);

        juce::dsp::AudioBlock<float> block (buf);
        dist.process (block);

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < kBlockSize; ++i)
            {
                const float s = buf.getSample (ch, i);

                if (! std::isfinite (s))
                    r.finite = false;

                r.peak = juce::jmax (r.peak, std::abs (s));
                r.audio.push_back (s);
            }
    }

    return r;
}

float maxAbsDiff (const std::vector<float>& a, const std::vector<float>& b)
{
    if (a.size() != b.size())
        return std::numeric_limits<float>::infinity();

    float m = 0.0f;
    for (size_t i = 0; i < a.size(); ++i)
        m = juce::jmax (m, std::abs (a[i] - b[i]));
    return m;
}

// The SHIPPED configuration, named once. [C] and [D] drive the replica with
// these, so if DistortionProcessor's constructor changes and these do not, [C]
// fails rather than quietly comparing against the wrong thing.
constexpr size_t kShippedFactor = 2; // 2^2 = 4x
const auto kShippedFilter = juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple;

// The pre-fix configuration through v1.28.1: naive Fold, polyphase IIR, 2x.
constexpr size_t kPrefixFactor = 1;
const auto kPrefixFilter = juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR;

// The rejected intermediate, kept so [I] can show WHY the factor had to move too:
// FIR at 2x fixes Fold's top octave but regresses the saturators at low pitch.
constexpr size_t kRejectedFactor = 1;

// Off-bin fundamentals across the range where the defect lives: at 1.46 Hz bins
// none of these is an exact bin multiple, so an alias of harmonic k cannot hide
// on top of harmonic j.
const double kFundamentals[] = { 110.7, 440.7, 1000.7, 4000.7 };

// 0.5 is where the one factory preset that selects Fold sits ("Fold Engine");
// 1.0 is the worst case the parameter allows.
const float kDrives[] = { 0.5f, 1.0f };

const Mode kSaturators[] = { Mode::SoftClip, Mode::HardClip, Mode::Tube };
} // namespace

//==============================================================================
int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    std::cout << "\n=== O-Prism distortion aliasing gate (IN-09, v1.29.0) ===\n";
    std::cout << std::fixed << std::setprecision (1);

    // ───────────────────────────────────────────────────────────────
    std::cout << "\n[A] Sanity — excitation is clean, every mode renders finite and audible\n";

    for (const double f0 : kFundamentals)
    {
        // The input's own alias floor. Everything measured later has to stand
        // clear of this or it is measuring the excitation, not the shaper.
        const auto excitation = bandLimitedSaw (f0, kFftSize + kBlockSize, kSampleRate);
        const double inputFloor = aliasToSignalDb (excitation, f0, kSampleRate);

        check (inputFloor < -80.0,
               "[A] @ " + db (f0, 1) + " Hz the band-limited saw's own alias floor is "
                   + db (inputFloor) + " dB — far under anything the shapers produce");
    }

    for (const double f0 : kFundamentals)
        for (const float drive : kDrives)
            for (const Mode m : { Mode::SoftClip, Mode::HardClip, Mode::Tube, Mode::Fold })
            {
                const auto r = renderThroughPlugin (m, drive, f0);

                check (r.finite && r.peak > 0.01f,
                       "[A] " + modeName (m) + " @ " + db (f0, 1) + " Hz drive " + db (drive, 2)
                           + " renders finite and non-silent (peak " + db (r.peak, 4) + ")");
            }

    // ───────────────────────────────────────────────────────────────
    std::cout << "\n[B] THE FINDING — Fold must beat the pre-fix path by "
              << kMinImprovementDb << " dB everywhere\n";

    for (const float drive : kDrives)
    {
        std::cout << "  -- drive " << drive << " (pre-gain x" << (1.0f + drive * 9.0f) << ") --\n";

        for (const double f0 : kFundamentals)
        {
            const double fold = aliasToSignalDb (renderThroughPlugin (Mode::Fold, drive, f0).audio,
                                                 f0, kSampleRate);

            // The built-in negative control: exactly what shipped through v1.28.1.
            const double prefix = aliasToSignalDb (
                renderReplica (Mode::Fold, drive, f0, kPrefixFactor, false, true, kPrefixFilter).audio,
                f0, kSampleRate);

            const double gain = prefix - fold;

            // Context, not a bound — see kMinImprovementDb.
            double worstSibling = -1000.0;
            juce::String worstName;

            for (const Mode m : kSaturators)
            {
                const double a = aliasToSignalDb (renderThroughPlugin (m, drive, f0).audio,
                                                  f0, kSampleRate);
                if (a > worstSibling)
                {
                    worstSibling = a;
                    worstName    = modeName (m);
                }
            }

            check (gain >= kMinImprovementDb,
                   "[B] @ " + db (f0, 1) + " Hz Fold " + db (fold) + " dB beats pre-fix "
                       + db (prefix) + " dB by " + db (gain) + " dB   [context: worst saturator "
                       + worstName + " " + db (worstSibling) + " dB]");
        }
    }

    // ───────────────────────────────────────────────────────────────
    std::cout << "\n[C] The replica tracks the source — at the shipped config, bit for bit\n";

    for (const float drive : kDrives)
    {
        const auto shipped = renderThroughPlugin (Mode::Fold, drive, 440.7);
        const auto replica = renderReplica (Mode::Fold, drive, 440.7,
                                            kShippedFactor, true, true, kShippedFilter);

        const float d = maxAbsDiff (shipped.audio, replica.audio);

        check (d == 0.0f,
               "[C] Fold @ drive " + db (drive, 2)
                   + ": the in-gate replica reproduces DistortionProcessor exactly (max |diff| "
                   + juce::String (d, 9) + ") — so [B]'s pre-fix baseline is the real pre-fix path");
    }

    // ───────────────────────────────────────────────────────────────
    std::cout << "\n[D] The other three modes take no ADAA — bit-identical to naive arithmetic\n";

    for (const Mode m : kSaturators)
        for (const float drive : kDrives)
        {
            const auto shipped = renderThroughPlugin (m, drive, 440.7);
            const auto replica = renderReplica (m, drive, 440.7, kShippedFactor, false,
                                                true, kShippedFilter);

            const float d = maxAbsDiff (shipped.audio, replica.audio);

            check (d == 0.0f,
                   "[D] " + modeName (m) + " @ drive " + db (drive, 2)
                       + " is bit-identical to its naive arithmetic (max |diff| "
                       + juce::String (d, 9) + ") — the ADAA branch is Fold-only");
        }

    // ───────────────────────────────────────────────────────────────
    std::cout << "\n[E] The ill-conditioned ADAA branch is safe, and the trap is armed\n";

    {
        const int n = kBlockSize * 4;

        std::vector<float> silence (static_cast<size_t> (n), 0.0f);

        std::vector<float> dc (static_cast<size_t> (n), 0.37f);

        std::vector<float> step (static_cast<size_t> (n), -1.0f);
        for (int i = n / 2; i < n; ++i)
            step[static_cast<size_t> (i)] = 1.0f;

        std::vector<float> square (static_cast<size_t> (n));
        for (int i = 0; i < n; ++i)
            square[static_cast<size_t> (i)] = (i / 64) % 2 == 0 ? 1.0f : -1.0f;

        const std::array<std::pair<const char*, const std::vector<float>*>, 4> cases {{
            { "silence (du == 0 every sample)", &silence },
            { "DC",                             &dc },
            { "a full-scale step",              &step },
            { "a full-scale square",            &square },
        }};

        for (const auto& c : cases)
            for (const float drive : { 0.0f, 0.5f, 1.0f })
            {
                const auto r = renderWaveform (Mode::Fold, drive, *c.second);

                check (r.finite,
                       juce::String ("[E] Fold on ") + c.first + " @ drive " + db (drive, 2)
                           + " renders finite");
            }

        // Silence must be silent, not merely finite: the fallback has to return
        // sin(0) exactly, not a denormal or a residue of the previous state.
        const auto sil = renderWaveform (Mode::Fold, 1.0f, silence);
        check (sil.peak == 0.0f,
               "[E] Fold on silence is exactly silent (peak " + juce::String (sil.peak, 9) + ")");

        // The trap, armed: strip the epsilon guard and the same inputs divide by
        // zero. If this ever passes, the guard has stopped being load-bearing and
        // [E] above has stopped proving anything.
        const auto unguarded = renderReplica (Mode::Fold, 1.0f, 440.7, kShippedFactor,
                                              true, false, kShippedFilter);
        check (! unguarded.finite,
               "[E] without the epsilon guard the quotient goes non-finite — "
               "the guard in DistortionProcessor is load-bearing");
    }

    // ───────────────────────────────────────────────────────────────
    std::cout << "\n[F] reset() clears the ADAA history\n";

    {
        const int n = kBlockSize * 2;

        std::vector<float> ramp (static_cast<size_t> (n));
        for (int i = 0; i < n; ++i)
            ramp[static_cast<size_t> (i)] = static_cast<float> (std::sin (2.0 * kPi * 440.0 * i / kSampleRate));

        DistortionProcessor dist;
        dist.prepare ({ kSampleRate, static_cast<juce::uint32> (kBlockSize), 2 });
        dist.setType (static_cast<int> (Mode::Fold));
        dist.setDrive (1.0f);
        dist.setMix (1.0f);

        auto renderOnce = [&]
        {
            std::vector<float> out;
            juce::AudioBuffer<float> buf (2, kBlockSize);

            for (int b = 0; b < n / kBlockSize; ++b)
            {
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < kBlockSize; ++i)
                        buf.setSample (ch, i, ramp[static_cast<size_t> (b * kBlockSize + i)]);

                juce::dsp::AudioBlock<float> block (buf);
                dist.process (block);

                for (int i = 0; i < kBlockSize; ++i)
                    out.push_back (buf.getSample (0, i));
            }
            return out;
        };

        const auto first = renderOnce();
        dist.reset();
        const auto second = renderOnce();

        check (maxAbsDiff (first, second) == 0.0f,
               "[F] a render after reset() is bit-identical to the first (max |diff| "
                   + juce::String (maxAbsDiff (first, second), 9) + ")");
    }

    // ───────────────────────────────────────────────────────────────
    // Diagnostic only, no assertions. [B] showed that once Fold is antialiased
    // the remaining alias is set by the DECIMATION FILTER, not the shaper:
    // polyphase IIR half-band has a wide transition band, so alias landing just
    // above the base Nyquist returns nearly unattenuated. This sweep is what the
    // shipped configuration was chosen from, and it is kept so the choice can be
    // re-derived rather than taken on trust.
    std::cout << "\n[G] Diagnostic — alias vs (factor, decimation filter), Fold + ADAA\n";

    {
        struct Cfg { const char* name; size_t factor; juce::dsp::Oversampling<float>::FilterType type; };

        const Cfg cfgs[] = {
            { "2x IIR",  1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR },
            { "4x IIR",  2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR },
            { "8x IIR",  3, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR },
            { "2x FIR",  1, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple },
            { "4x FIR",  2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple },
            { "8x FIR",  3, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple },
        };

        std::cout << "  -- reported latency at 48 kHz (samples), which the host sees --\n            ";
        for (const auto& c : cfgs)
        {
            juce::dsp::Oversampling<float> os (2, c.factor, c.type, true);
            os.initProcessing (static_cast<size_t> (kBlockSize));
            std::cout << std::setw (10) << os.getLatencyInSamples();
        }
        std::cout << "\n";

        for (const float drive : kDrives)
        {
            std::cout << "  -- drive " << drive << " --\n            ";
            for (const auto& c : cfgs)
                std::cout << std::setw (10) << c.name;
            std::cout << std::setw (14) << "worst sib\n";

            for (const double f0 : kFundamentals)
            {
                std::cout << "    " << std::setw (8) << f0 << "  ";

                for (const auto& c : cfgs)
                {
                    const double a = aliasToSignalDb (
                        renderReplica (Mode::Fold, drive, f0, c.factor, true, true, c.type).audio,
                        f0, kSampleRate);
                    std::cout << std::setw (10) << a;
                }

                double worstSibling = -1000.0;
                for (const Mode m : kSaturators)
                    worstSibling = std::max (worstSibling,
                                             aliasToSignalDb (renderThroughPlugin (m, drive, f0).audio,
                                                              f0, kSampleRate));
                std::cout << std::setw (14) << worstSibling << "\n";
            }
        }
    }

    // ───────────────────────────────────────────────────────────────
    // Diagnostic only. [B] measures the alias REMOVED; this measures how far the
    // partial spectrum MOVED, which is what an existing Fold patch will hear. The
    // two are independent: a change could remove alias without touching the
    // partials, or move the partials without removing any alias.
    std::cout << "\n[H] Diagnostic — how far Fold's partial spectrum moves vs the pre-fix path\n";

    for (const float drive : kDrives)
    {
        std::cout << "  -- drive " << drive << " --\n";

        for (const double f0 : kFundamentals)
        {
            const auto shipped = renderThroughPlugin (Mode::Fold, drive, f0);
            const auto prefix  = renderReplica (Mode::Fold, drive, f0, kPrefixFactor,
                                                false, true, kPrefixFilter);

            const double dev = partialDeviationDb (shipped.audio, prefix.audio, f0, kSampleRate);

            std::cout << "        @ " << std::setw (8) << f0 << " Hz  partial deviation "
                      << std::setw (7) << dev << " dB"
                      << (dev < -20.0 ? "   (subtle)" : dev < -10.0 ? "   (audible)"
                                                                    : "   (clearly different)")
                      << "\n";
        }
    }

    // ───────────────────────────────────────────────────────────────
    std::cout << "\n[I] NON-REGRESSION — the decimator change must not make the saturators worse\n";

    for (const Mode m : kSaturators)
        for (const float drive : kDrives)
            for (const double f0 : kFundamentals)
            {
                const double now = aliasToSignalDb (renderThroughPlugin (m, drive, f0).audio,
                                                   f0, kSampleRate);
                const double before = aliasToSignalDb (
                    renderReplica (m, drive, f0, kPrefixFactor, false, true, kPrefixFilter).audio,
                    f0, kSampleRate);
                // The rejected intermediate, printed as the reason the factor moved.
                const double fir2x = aliasToSignalDb (
                    renderReplica (m, drive, f0, kRejectedFactor, false, true, kShippedFilter).audio,
                    f0, kSampleRate);

                check (now <= before + kSaturatorRegressionToleranceDb,
                       "[I] " + modeName (m) + " @ " + db (f0, 1) + " Hz drive " + db (drive, 2)
                           + ": " + db (now) + " dB vs pre-fix " + db (before) + " dB ("
                           + (now <= before ? "improved by " + db (before - now)
                                            : "WORSE by " + db (now - before))
                           + " dB)   [2x FIR, the rejected option, would be " + db (fir2x)
                           + " dB]");
            }

    // ───────────────────────────────────────────────────────────────
    // Diagnostic only. The v1.29.0 configuration costs more CPU than the one it
    // replaces — 4x instead of 2x, and an equiripple FIR instead of a polyphase
    // IIR — and this quantifies it rather than leaving it as "roughly double".
    // The excitation is generated once, outside the timed region, so what is timed
    // is process() and nothing else.
    std::cout << "\n[J] Diagnostic — CPU cost of the stage, shipped vs pre-fix\n";

    {
        constexpr int kTimedBlocks = 2000;
        const auto excitation = bandLimitedSaw (440.7, kBlockSize, kSampleRate);

        juce::AudioBuffer<float> buf (2, kBlockSize);

        auto fill = [&]
        {
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < kBlockSize; ++i)
                    buf.setSample (ch, i, excitation[static_cast<size_t> (i)]);
        };

        // (a) the real processor, shipped configuration.
        auto timeShipped = [&] (Mode m)
        {
            DistortionProcessor dist;
            dist.prepare ({ kSampleRate, static_cast<juce::uint32> (kBlockSize), 2 });
            dist.setType (static_cast<int> (m));
            dist.setDrive (1.0f);
            dist.setMix (1.0f);

            fill();
            { juce::dsp::AudioBlock<float> warm (buf); dist.process (warm); } // page in

            const auto t0 = std::chrono::steady_clock::now();
            for (int b = 0; b < kTimedBlocks; ++b)
            {
                fill();
                juce::dsp::AudioBlock<float> block (buf);
                dist.process (block);
            }
            return std::chrono::duration<double, std::micro> (
                       std::chrono::steady_clock::now() - t0).count() / kTimedBlocks;
        };

        // (b) the pre-fix arithmetic and decimator, timed on the same shape.
        auto timePrefix = [&] (Mode m)
        {
            juce::dsp::Oversampling<float> os (2, kPrefixFactor, kPrefixFilter, true);
            os.initProcessing (static_cast<size_t> (kBlockSize));
            juce::dsp::DryWetMixer<float> mixer;
            mixer.prepare ({ kSampleRate, static_cast<juce::uint32> (kBlockSize), 2 });
            mixer.setWetLatency (static_cast<float> (os.getLatencyInSamples()));
            mixer.setWetMixProportion (1.0f);

            const float gain = 10.0f;

            auto oneBlock = [&]
            {
                fill();
                juce::dsp::AudioBlock<float> block (buf);
                mixer.pushDrySamples (block);
                auto osBlock = os.processSamplesUp (block);

                for (size_t ch = 0; ch < osBlock.getNumChannels(); ++ch)
                {
                    auto* data = osBlock.getChannelPointer (ch);
                    for (size_t i = 0; i < osBlock.getNumSamples(); ++i)
                    {
                        const float x = data[i] * gain;
                        switch (m)
                        {
                            case Mode::SoftClip: data[i] = std::tanh (x); break;
                            case Mode::HardClip: data[i] = juce::jlimit (-1.0f, 1.0f, x); break;
                            case Mode::Tube:
                                data[i] = x >= 0.0f ? x / (1.0f + std::abs (x))
                                                    : std::tanh (x * 1.5f) / 1.5f;
                                break;
                            case Mode::Fold:
                                data[i] = static_cast<float> (std::sin (x * kPi));
                                break;
                        }
                    }
                }

                os.processSamplesDown (block);
                mixer.mixWetSamples (block);
            };

            oneBlock();

            const auto t0 = std::chrono::steady_clock::now();
            for (int b = 0; b < kTimedBlocks; ++b)
                oneBlock();
            return std::chrono::duration<double, std::micro> (
                       std::chrono::steady_clock::now() - t0).count() / kTimedBlocks;
        };

        // Any (factor, filter) pair, ADAA on Fold, so the candidates compare directly.
        auto timeCfg = [&] (Mode m, size_t factor,
                            juce::dsp::Oversampling<float>::FilterType type, bool adaa)
        {
            juce::dsp::Oversampling<float> os (2, factor, type, true);
            os.initProcessing (static_cast<size_t> (kBlockSize));
            juce::dsp::DryWetMixer<float> mixer;
            mixer.prepare ({ kSampleRate, static_cast<juce::uint32> (kBlockSize), 2 });
            mixer.setWetLatency (static_cast<float> (os.getLatencyInSamples()));
            mixer.setWetMixProportion (1.0f);

            const float gain = 10.0f;
            double prevU[2] { 0.0, 0.0 };

            auto oneBlock = [&]
            {
                fill();
                juce::dsp::AudioBlock<float> block (buf);
                mixer.pushDrySamples (block);
                auto osBlock = os.processSamplesUp (block);

                for (size_t ch = 0; ch < osBlock.getNumChannels(); ++ch)
                {
                    auto* data = osBlock.getChannelPointer (ch);
                    double pv = prevU[ch];

                    for (size_t i = 0; i < osBlock.getNumSamples(); ++i)
                    {
                        const float x = data[i] * gain;
                        switch (m)
                        {
                            case Mode::SoftClip: data[i] = std::tanh (x); break;
                            case Mode::HardClip: data[i] = juce::jlimit (-1.0f, 1.0f, x); break;
                            case Mode::Tube:
                                data[i] = x >= 0.0f ? x / (1.0f + std::abs (x))
                                                    : std::tanh (x * 1.5f) / 1.5f;
                                break;
                            case Mode::Fold:
                                if (! adaa)
                                {
                                    data[i] = static_cast<float> (std::sin (x * kPi));
                                }
                                else
                                {
                                    const double u  = static_cast<double> (x);
                                    const double du = u - pv;
                                    const auto   F  = [] (double v) { return -std::cos (v * kPi) / kPi; };
                                    data[i] = static_cast<float> (std::abs (du) > 1.0e-7
                                        ? (F (u) - F (pv)) / du
                                        : std::sin (0.5 * (u + pv) * kPi));
                                }
                                break;
                        }
                        pv = static_cast<double> (x);
                    }

                    prevU[ch] = pv;
                }

                os.processSamplesDown (block);
                mixer.mixWetSamples (block);
            };

            oneBlock();
            const auto t0 = std::chrono::steady_clock::now();
            for (int b = 0; b < kTimedBlocks; ++b)
                oneBlock();
            return std::chrono::duration<double, std::micro> (
                       std::chrono::steady_clock::now() - t0).count() / kTimedBlocks;
        };

        std::cout << std::setprecision (2);
        std::cout << "        (us per " << kBlockSize << "-sample stereo block, drive 1.0;"
                  << " one block is " << (1000.0 * kBlockSize / kSampleRate) << " ms of audio)\n";

        for (const Mode m : { Mode::SoftClip, Mode::HardClip, Mode::Tube, Mode::Fold })
        {
            const double before = timePrefix (m);
            const double now    = timeShipped (m);

            std::cout << "        " << std::setw (10) << modeName (m).toRawUTF8()
                      << "  pre-fix " << std::setw (7) << before
                      << "  shipped " << std::setw (7) << now
                      << "   x" << std::setw (5) << (now / before)
                      << "   (" << std::setw (5)
                      << (100.0 * now / (1000.0 * kBlockSize / kSampleRate) / 1000.0)
                      << " % of one core)\n";
        }

        std::cout << "\n        candidate sweep, Fold with ADAA, us per block:\n";

        struct TCfg { const char* name; size_t factor; juce::dsp::Oversampling<float>::FilterType type; };
        const TCfg tcfgs[] = {
            { "2x IIR", 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR },
            { "4x IIR", 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR },
            { "2x FIR", 1, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple },
            { "4x FIR", 2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple },
        };

        const double base = timePrefix (Mode::Fold);

        for (const auto& c : tcfgs)
        {
            const double t = timeCfg (Mode::Fold, c.factor, c.type, true);
            std::cout << "        " << std::setw (10) << c.name
                      << "  " << std::setw (7) << t
                      << "   x" << std::setw (5) << (t / base) << " vs pre-fix\n";
        }

        std::cout << std::setprecision (1);
    }

    // ───────────────────────────────────────────────────────────────
    std::cout << "\n" << (failures == 0 ? "ALL CHECKS PASSED"
                                        : "FAILURES: " + std::to_string (failures))
              << "\n\n";

    return failures;
}
