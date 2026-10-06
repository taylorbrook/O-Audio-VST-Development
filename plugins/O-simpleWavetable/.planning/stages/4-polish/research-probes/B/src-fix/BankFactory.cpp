/*
   This file is part of O-simpleWavetable, an Ouaricon Audio plugin.
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

    O-simpleWavetable - BankFactory implementation (Stage 2.1)

  ==============================================================================
*/

#include "BankFactory.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace
{
    constexpr int    kN       = MipmapBuilder::kN;        // 2048
    constexpr int    kPacked  = MipmapBuilder::kPacked;   // 4096
    constexpr int    kMaxHarm = 1023;
    constexpr double kPi      = 3.14159265358979323846;

    // sine amplitude b_n -> d[2n+1]; cosine amplitude a_n -> d[2n]
    void addSine (std::vector<float>& d, int n, double b)
    {
        d[(size_t) (2 * n + 1)] = (float) (-0.5 * (double) kN * b);
    }

    void addCosine (std::vector<float>& d, int n, double a)
    {
        d[(size_t) (2 * n)] = (float) (0.5 * (double) kN * a);
    }

    void clearPacked (std::vector<float>& d)
    {
        std::fill (d.begin(), d.end(), 0.0f);
    }

    //==========================================================================
    void buildSineSaw (WavetableBank& out, MipmapBuilder& mb, std::vector<float>& d)
    {
        for (int k = 1; k <= BankFactory::kFrames; ++k)
        {
            BankFactory::fillSawSpectrum (d.data(), k);
            mb.buildFromSpectrum (out, k - 1, d.data(), MipmapBuilder::Normalise::peakToUnity);
        }
    }

    void buildSineSquare (WavetableBank& out, MipmapBuilder& mb, std::vector<float>& d)
    {
        for (int k = 1; k <= BankFactory::kFrames; ++k)
        {
            clearPacked (d);
            const double c = BankFactory::squareFrontier (k);
            for (int j = 1; j <= 16; ++j)
            {
                const int n = 2 * j - 1;
                const double w = juce::jlimit (0.0, 1.0, c - (double) (j - 1));
                if (w > 0.0)
                    addSine (d, n, w / (double) n);
            }
            mb.buildFromSpectrum (out, k - 1, d.data(), MipmapBuilder::Normalise::peakToUnity);
        }
    }

    void buildPulseWidth (WavetableBank& out, MipmapBuilder& mb, std::vector<float>& d)
    {
        for (int k = 1; k <= BankFactory::kFrames; ++k)
        {
            clearPacked (d);
            const double duty = BankFactory::pulseDuty (k);
            for (int n = 1; n <= kMaxHarm; ++n)
            {
                // Pulse centred at phi = 0.5: cos(2 pi n (phi - 0.5)) = (-1)^n cos(2 pi n phi)
                const double a    = (2.0 / ((double) n * kPi)) * std::sin ((double) n * kPi * duty);
                const double sign = (n % 2 == 0) ? 1.0 : -1.0;
                addCosine (d, n, a * sign);
            }
            mb.buildFromSpectrum (out, k - 1, d.data(), MipmapBuilder::Normalise::peakToUnity);
        }
    }

    void buildFormant (WavetableBank& out, MipmapBuilder& mb, std::vector<float>& d)
    {
        for (int k = 1; k <= BankFactory::kFrames; ++k)
        {
            clearPacked (d);
            const auto f = BankFactory::formantsAt ((double) k);
            for (int n = 1; n <= BankFactory::kFormantHarmonics; ++n)
                addSine (d, n, BankFactory::formantAmplitude (n, f));
            mb.buildFromSpectrum (out, k - 1, d.data(), MipmapBuilder::Normalise::peakToUnity);
        }
    }

    void buildDrive (WavetableBank& out, MipmapBuilder& mb)
    {
        std::vector<float> x ((size_t) kN, 0.0f);
        for (int k = 1; k <= BankFactory::kFrames; ++k)
        {
            const double g    = BankFactory::driveGain (k);
            const double norm = 1.0 / std::tanh (g);
            for (int i = 0; i < kN; ++i)
            {
                const double s = std::sin (2.0 * kPi * (double) i / (double) kN);
                x[(size_t) i] = (float) (std::tanh (g * s) * norm);
            }
            mb.buildFromTime (out, k - 1, x.data(), MipmapBuilder::Normalise::peakToUnity);
        }
    }

    // Peterson & Barney (1952) adult-male averages (ARCHITECTURE A7).
    constexpr std::array<BankFactory::Formants, BankFactory::kNumVowels> kVowels {{
        { 730.0, 1090.0, 2440.0 },   // A
        { 530.0, 1840.0, 2480.0 },   // E
        { 270.0, 2290.0, 3010.0 },   // I
        { 570.0,  840.0, 2410.0 },   // O
        { 300.0,  870.0, 2240.0 }    // U
    }};

    constexpr std::array<double, BankFactory::kNumVowels> kVowelFrames { 1.0, 8.75, 16.5, 24.25, 32.0 };

    constexpr double kBandwidth1 = 90.0;
    constexpr double kBandwidth2 = 110.0;
    constexpr double kBandwidth3 = 170.0;

    double resonator (double f, double fc, double bw)
    {
        const double h  = 0.5 * bw;
        const double h2 = h * h;
        const double num = fc * fc + h2;
        const double den = std::sqrt (((f - fc) * (f - fc) + h2) * ((f + fc) * (f + fc) + h2));
        return den > 0.0 ? num / den : 0.0;
    }

    double logLerp (double a, double b, double t)
    {
        return std::exp ((1.0 - t) * std::log (a) + t * std::log (b));
    }
}

//==============================================================================
namespace BankFactory
{
    double squareFrontier (int k) noexcept
    {
        return 1.0 + 15.0 * (double) (k - 1) / 31.0;
    }

    double pulseDuty (int k) noexcept
    {
        return 0.5 * std::pow (1.0 / 16.0, (double) (k - 1) / 31.0);
    }

    double driveGain (int k) noexcept
    {
        return 0.25 * std::pow (100.0, (double) (k - 1) / 31.0);
    }

    Formants vowelAnchor (int vowel) noexcept
    {
        return kVowels[(size_t) juce::jlimit (0, kNumVowels - 1, vowel)];
    }

    double vowelAnchorFrame (int vowel) noexcept
    {
        return kVowelFrames[(size_t) juce::jlimit (0, kNumVowels - 1, vowel)];
    }

    Formants formantsAt (double frameK) noexcept
    {
        if (! (frameK > kVowelFrames[0]))
            return kVowels[0];
        if (frameK >= kVowelFrames[(size_t) (kNumVowels - 1)])
            return kVowels[(size_t) (kNumVowels - 1)];

        int seg = 0;
        while (seg < kNumVowels - 2 && frameK > kVowelFrames[(size_t) (seg + 1)])
            ++seg;

        const double p0 = kVowelFrames[(size_t) seg];
        const double p1 = kVowelFrames[(size_t) (seg + 1)];
        const double t  = (frameK - p0) / (p1 - p0);
        const auto& a = kVowels[(size_t) seg];
        const auto& b = kVowels[(size_t) (seg + 1)];

        // Exact anchors at the segment ends (no exp(log(x)) rounding).
        if (t <= 0.0) return a;
        if (t >= 1.0) return b;

        return { logLerp (a.f1, b.f1, t), logLerp (a.f2, b.f2, t), logLerp (a.f3, b.f3, t) };
    }

    double formantAmplitude (int n, const Formants& f) noexcept
    {
        const double hz = (double) n * kFormantRefHz;
        return (1.0 / (double) n)
             * resonator (hz, f.f1, kBandwidth1)
             * resonator (hz, f.f2, kBandwidth2)
             * resonator (hz, f.f3, kBandwidth3);
    }

    void fillSawSpectrum (float* packed, int numHarmonics) noexcept
    {
        std::fill (packed, packed + kPacked, 0.0f);
        const int top = juce::jlimit (0, kMaxHarm, numHarmonics);
        for (int n = 1; n <= top; ++n)
            packed[2 * n + 1] = (float) (-0.5 * (double) kN / (double) n);
    }

    const char* nameFor (int index) noexcept
    {
        switch (index)
        {
            case sineSaw:    return "Sine->Saw";
            case sineSquare: return "Sine->Square";
            case pulseWidth: return "Pulse Width";
            case formant:    return "Formant";
            case drive:      return "Drive";
            default:         return "Unknown";
        }
    }

    void build (int index, WavetableBank& out, MipmapBuilder& builder)
    {
        out.name = nameFor (index);
        out.allocate (kFrames);

        std::vector<float> d ((size_t) kPacked, 0.0f);

        switch (index)
        {
            case sineSaw:    buildSineSaw    (out, builder, d); break;
            case sineSquare: buildSineSquare (out, builder, d); break;
            case pulseWidth: buildPulseWidth (out, builder, d); break;
            case formant:    buildFormant    (out, builder, d); break;
            case drive:      buildDrive      (out, builder);    break;
            default:         jassertfalse;                      break;
        }
    }
}
