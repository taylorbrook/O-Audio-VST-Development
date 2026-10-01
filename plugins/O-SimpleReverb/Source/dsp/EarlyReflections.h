/*
   This file is part of O-SimpleReverb, an Ouaricon Audio plugin.
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

    O-SimpleReverb - early reflections
    Ouaricon Audio

    A stereo tap line for the FDN types (Booth, Room, Hall, Ambient). It is
    fed from the pre-delayed input, ahead of the tank's diffusion, and its
    output goes to the slot's OUTPUT: discrete arrivals before the late tail,
    at different times in L and R. Up to v1.14.0 the taps were summed into the
    tank's input, where eight combs each repeated them into a click train.

    Plate and Spring do not use it: a plate has no discrete early arrivals and
    a spring tank has none either.

  ==============================================================================
*/

#pragma once
#include "ReverbPrimitives.h"
#include <array>

namespace osr
{

class EarlyReflections
{
public:
    static constexpr int kTaps = 8;
    static constexpr double kMaxSizeScale = 2.0;

    // Tap times as a share of the span (the last left tap). No left tap is
    // within 0.03 of a right one, so at the smallest span in use (Booth at
    // SIZE 0, 3.5 ms) every tap is still 0.1 ms clear of the other side's.
    static constexpr std::array<float, kTaps> kTimesL = { 0.110f, 0.205f, 0.318f, 0.433f, 0.562f, 0.694f, 0.829f, 1.000f };
    static constexpr std::array<float, kTaps> kTimesR = { 0.143f, 0.247f, 0.279f, 0.471f, 0.523f, 0.741f, 0.873f, 0.951f };

    // maxSpanMs: the longest span any type sets, at size scale 1.
    void prepare(double sampleRate, float maxSpanMs)
    {
        fs = sampleRate;
        const int capacity = static_cast<int>(std::ceil(maxSpanMs * kMaxSizeScale * 0.001 * sampleRate)) + 2;
        lineL.prepare(capacity);
        lineR.prepare(capacity);
        glide = slewCoefficient(0.5 * kSizeGlideSeconds, sampleRate);

        // Gains fall as 1 / t with alternating polarity, each side scaled to
        // unit energy, so the host's level is the taps' level whatever the span.
        for (int side = 0; side < 2; ++side) {
            const auto& times = side == 0 ? kTimesL : kTimesR;
            auto& gains = side == 0 ? gainL : gainR;
            float energy = 0.0f;
            for (int k = 0; k < kTaps; ++k) {
                gains[static_cast<size_t>(k)] = (k % 2 == 0 ? 1.0f : -1.0f) * times[0] / times[static_cast<size_t>(k)];
                energy += gains[static_cast<size_t>(k)] * gains[static_cast<size_t>(k)];
            }
            for (auto& g : gains) g /= std::sqrt(energy);
        }
        setType(maxSpanMs);
        reset();
    }

    // Not under signal. Follow with setSize() and reset().
    void setType(float spanMilliseconds) { spanMs = spanMilliseconds; applyLengths(); }

    // The tap times glide to the new scale.
    void setSize(double scale)
    {
        scale = std::min(scale, kMaxSizeScale);
        if (sameValue(scale, sizeScale)) return;
        sizeScale = scale;
        applyLengths();
    }

    // Lands the taps on their targets without clearing anything.
    void land()
    {
        for (auto* taps : { &tapL, &tapR })
            for (auto& t : *taps) t.land();
    }

    // Clears the lines and lands the taps.
    void reset()
    {
        lineL.reset();
        lineR.reset();
        land();
    }

    void process(float inL, float inR, float& outL, float& outR)
    {
        outL = read(lineL, tapL, gainL);
        outR = read(lineR, tapR, gainR);
        lineL.push(inL);
        lineR.push(inR);
    }

    // Where tap k of a side (0 = left) is heading, in samples, and its gain.
    double tapSamples(int side, int k) const { return (side == 0 ? tapL : tapR)[static_cast<size_t>(k)].target; }
    float tapGain(int side, int k) const { return (side == 0 ? gainL : gainR)[static_cast<size_t>(k)]; }

private:
    using Taps = std::array<GlideLength, kTaps>;

    void applyLengths()
    {
        for (int k = 0; k < kTaps; ++k) {
            const double span = spanMs * sizeScale * 0.001 * fs;
            // Whole samples, so a settled tap is one sample and not a 4-point kernel.
            tapL[static_cast<size_t>(k)].set(std::max(2.0, std::round(kTimesL[static_cast<size_t>(k)] * span)));
            tapR[static_cast<size_t>(k)].set(std::max(2.0, std::round(kTimesR[static_cast<size_t>(k)] * span)));
        }
    }

    float read(const RingDelay& line, Taps& taps, const std::array<float, kTaps>& gains)
    {
        float sum = 0.0f;
        for (int k = 0; k < kTaps; ++k) {
            auto& t = taps[static_cast<size_t>(k)];
            if (t.moving) {
                t.tick(glide);
                sum += gains[static_cast<size_t>(k)] * line.readLagrange(t.current);
            } else {
                sum += gains[static_cast<size_t>(k)] * line.at(static_cast<int>(t.current));
            }
        }
        return sum;
    }

    RingDelay lineL, lineR;
    Taps tapL, tapR;
    std::array<float, kTaps> gainL {}, gainR {};
    double fs = 44100.0;
    double glide = 1.0;
    double sizeScale = 1.0;
    float spanMs = 20.0f;
};

} // namespace osr
