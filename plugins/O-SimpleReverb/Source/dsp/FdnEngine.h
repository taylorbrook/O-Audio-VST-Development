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

    O-SimpleReverb - feedback delay network (Booth, Room, Hall, Ambient)
    Ouaricon Audio

    Sixteen delay lines mixed through a Hadamard matrix. What makes one type
    different from another is its delay set (FdnConfig), not a damping setting
    on shared delays: two types' tail spectra correlate at about 0, where the
    one Freeverb tank every type ran through up to v1.14.0 gave 0.7-0.96.

      - Sixteen lines, not eight: Schroeder's mode-density rule wants a total
        delay of 0.15 x T60. Eight lines give Hall 0.38 s against 0.45 s
        needed; sixteen give 0.74 s.
      - Hadamard, not Householder: above four lines Householder is nearly
        diagonal and mixes slowly (mixing time 390 ms against 288 ms at N = 8).
      - Decay time is a number in seconds. Each line's gain is
        10^(-3 L / (fs T60)) from the line's CURRENT length, so the tail time
        holds while SIZE moves the lengths, glide included.
      - Two decay bands per line: gHi x + (gLo - gHi) LP(x). The high band's
        time is a voicing (hfRatio is nominal; the one-pole shelf reaches it
        only well above the crossover). The mid band is calibrated at 1 kHz.
      - True stereo: L and R enter through two Hadamard rows and leave through
        two others. No channel is a delayed copy of the other.

    LOOP GAIN: every line's |H| <= gLo <= 0.9999 (solveShelfLow's ceiling), the
    Hadamard matrix is orthogonal, and the Lagrange reads sit in their central
    interval (|H| <= 1). The loop cannot grow at any DECAY or SIZE.

    Modulation phases are fixed at setType(): no Random, no clock, nothing
    taken from `this`, so two renders of the same input are the same file.

  ==============================================================================
*/

#pragma once
#include "ReverbPrimitives.h"
#include <array>

namespace osr
{

struct FdnConfig
{
    float minMs;            // shortest and longest line at size scale 1; the
    float maxMs;            //   sixteen sit on a geometric ladder between them
    int   jitter;           // which +/-3.5 % irregularity the ladder gets (one per type)
    float hfRatio;          // nominal T60(high) / T60(mid) - a voicing, see above
    float crossoverHz;      // between the two decay bands
    float modMs;            // peak excursion of the modulated lines (0 = none)
    float modHz;            // centre rate; lines spread 0.6x..1.6x around it
    float diffusionScale;   // input allpass lengths, <= 1. An allpass rings for
                            //   3 D / -log10(g) seconds - 187 ms at 12.7 ms and
                            //   0.625 - which puts a floor under a short tail:
                            //   Booth at 0.5x read +15.8 % until it was 0.25.
};

class FdnEngine
{
public:
    static constexpr int kLines = 16;
    static constexpr double kMaxSizeScale = 2.0;
    static constexpr double kLadderIrregularity = 0.035;
    static constexpr double kMidReferenceHz = 1000.0;
    static constexpr double kDecayGlideSeconds = 0.06;   // DECAY moves: gains slew, they do not step
    static constexpr int kGainInterval = 16;             // samples between gain recomputes while anything moves

    // maxLineMs / maxModMs: the largest FdnConfig::maxMs and ::modMs any type will set.
    void prepare(double sampleRate, float maxLineMs, float maxModMs)
    {
        fs = sampleRate;
        const double longestMs = maxLineMs * kMaxSizeScale * (1.0 + kLadderIrregularity) + maxModMs;
        const int capacity = static_cast<int>(std::ceil(longestMs * 0.001 * sampleRate)) + 2;
        for (auto& line : lines)
            line.delay.prepare(capacity);

        const int diffuserCapacity = static_cast<int>(std::ceil(kDiffuserMsL[2] * 0.001 * sampleRate)) + 1;
        for (auto* bank : { &diffuserL, &diffuserR })
            for (auto& ap : *bank) ap.prepare(diffuserCapacity);

        // Hadamard rows as in/out vectors. Rows are orthogonal, so L and R
        // excite, and are read from, different mixtures of the sixteen lines.
        const float norm = 1.0f / std::sqrt(static_cast<float>(kLines));
        for (int i = 0; i < kLines; ++i) {
            const auto row = [i](int r) { return (parity(r & i) ? -1.0f : 1.0f); };
            inL[static_cast<size_t>(i)]  = row(1) * norm * 1.41421356f;
            inR[static_cast<size_t>(i)]  = row(2) * norm * 1.41421356f;
            outL[static_cast<size_t>(i)] = row(kLines / 2 + 1) * norm;
            outR[static_cast<size_t>(i)] = row(kLines / 2 + 2) * norm;
        }

        lengthGlide = slewCoefficient(0.5 * kSizeGlideSeconds, sampleRate);
        decayGlide = slewCoefficient(kDecayGlideSeconds, sampleRate);
    }

    // Not under signal: the delay set jumps. Follow with setSize(), setT60()
    // and reset().
    void setType(const FdnConfig& config)
    {
        cfg = config;
        const double span = static_cast<double>(cfg.maxMs) / cfg.minMs;
        for (int i = 0; i < kLines; ++i) {
            auto& line = lines[static_cast<size_t>(i)];
            const double u = static_cast<double>(i) / (kLines - 1);
            // golden-angle hash: deterministic, different for every `jitter`
            const double wobble = kLadderIrregularity * std::sin(2.39996 * (i + 1) * (cfg.jitter + 1));
            line.baseSamples = cfg.minMs * std::pow(span, u) * (1.0 + wobble) * 0.001 * fs;
            line.modulated = cfg.modMs > 0.0f && cfg.modHz > 0.0f && (i % 2 == 1);

            const double rate = cfg.modHz * (0.6 + static_cast<double>((i * 7) % kLines) / kLines);
            const double phase = 2.0 * kPi * static_cast<double>((i * 5) % kLines) / kLines;
            line.oscStep = 2.0 * std::sin(kPi * rate / fs);
            line.oscSinStart = std::sin(phase);
            line.oscCosStart = std::cos(phase);
        }
        modDepth = cfg.modMs * 0.001 * fs;

        shelfPole = static_cast<float>(std::exp(-2.0 * kPi * cfg.crossoverHz / fs));
        shelfRef = shelfReference(shelfPole, kMidReferenceHz, fs);

        const float scale = std::min(1.0f, cfg.diffusionScale);
        for (int k = 0; k < 4; ++k) {
            const float g = k < 2 ? 0.75f : 0.625f;
            diffuserL[static_cast<size_t>(k)].set(static_cast<int>(std::lround(kDiffuserMsL[k] * scale * 0.001 * fs)), g);
            diffuserR[static_cast<size_t>(k)].set(static_cast<int>(std::lround(kDiffuserMsR[k] * scale * 0.001 * fs)), g);
        }
        applyLengths();
    }

    // Length scale, 1 = the config's own lengths. The lines glide there.
    void setSize(double scale)
    {
        scale = std::min(scale, kMaxSizeScale);
        if (sameValue(scale, sizeScale)) return;
        sizeScale = scale;
        applyLengths();
    }

    // Mid-band decay time in seconds. The gains slew there.
    void setT60(double seconds)
    {
        seconds = std::max(0.05, seconds);
        if (sameValue(seconds, t60Target)) return;
        t60Target = seconds;
        t60Moving = true;
        gainsLive = true;
    }

    // Lands every glide on its target without clearing anything.
    void land()
    {
        for (auto& line : lines) {
            line.length.land();
            line.whole = static_cast<int>(line.length.current);
        }
        t60 = t60Target;
        t60Moving = false;
        computeGains();
        for (auto& line : lines) {
            line.gLo = line.gLoTarget; line.gHi = line.gHiTarget;
            line.gLoStep = line.gHiStep = 0.0f;
        }
        gainsLive = false;
        gainsSettling = false;
        gainCountdown = 0;
    }

    // Clears the tank and lands the glides.
    void reset()
    {
        for (auto& line : lines) {
            line.delay.reset();
            line.shelf = 0.0f;
            line.oscSin = line.oscSinStart;
            line.oscCos = line.oscCosStart;
        }
        for (auto* bank : { &diffuserL, &diffuserR })
            for (auto& ap : *bank) ap.reset();
        land();
    }

    void process(float l, float r, float& yL, float& yR)
    {
        for (int k = 0; k < 4; ++k) {
            l = diffuserL[static_cast<size_t>(k)].process(l);
            r = diffuserR[static_cast<size_t>(k)].process(r);
        }

        if (t60Moving) {
            t60 += (t60Target - t60) * decayGlide;
            if (std::abs(t60Target - t60) < 1.0e-4 * t60Target) { t60 = t60Target; t60Moving = false; }
        }
        if (gainsLive) {
            if (gainCountdown == 0) retargetGains();
            --gainCountdown;
        }

        std::array<float, kLines> v;
        float sumL = 0.0f, sumR = 0.0f;
        for (int i = 0; i < kLines; ++i) {
            auto& line = lines[static_cast<size_t>(i)];
            if (line.length.moving) {
                line.length.tick(lengthGlide);
                if (! line.length.moving) line.whole = static_cast<int>(line.length.current);
            }

            float x;
            if (line.modulated) {
                line.oscSin += line.oscStep * line.oscCos;   // coupled form: bounded, no sin() per sample
                line.oscCos -= line.oscStep * line.oscSin;
                x = line.delay.readLagrange(line.length.current + modDepth * line.oscSin);
            } else if (line.length.moving) {
                x = line.delay.readLagrange(line.length.current);
            } else {
                x = line.delay.at(line.whole);
            }

            line.gLo += line.gLoStep;
            line.gHi += line.gHiStep;
            line.shelf = x + shelfPole * (line.shelf - x);
            v[static_cast<size_t>(i)] = line.gHi * x + (line.gLo - line.gHi) * line.shelf;   // |H| <= gLo < 1
            sumL += outL[static_cast<size_t>(i)] * x;
            sumR += outR[static_cast<size_t>(i)] * x;
        }

        // fast Walsh-Hadamard, then 1 / sqrt(16): orthogonal
        for (int h = 1; h < kLines; h <<= 1)
            for (int i = 0; i < kLines; i += h << 1)
                for (int j = i; j < i + h; ++j) {
                    const float a = v[static_cast<size_t>(j)], b = v[static_cast<size_t>(j + h)];
                    v[static_cast<size_t>(j)] = a + b;
                    v[static_cast<size_t>(j + h)] = a - b;
                }

        for (int i = 0; i < kLines; ++i)
            lines[static_cast<size_t>(i)].delay.push(0.25f * v[static_cast<size_t>(i)]
                                                     + inL[static_cast<size_t>(i)] * l + inR[static_cast<size_t>(i)] * r);
        yL = sumL;
        yR = sumR;
    }

    // Where line i is heading, in samples, and the sum over the lines (seconds).
    double lineSamples(int i) const { return lines[static_cast<size_t>(i)].length.target; }
    double totalDelaySeconds() const
    {
        double sum = 0.0;
        for (const auto& line : lines) sum += line.length.target;
        return sum / fs;
    }

private:
    struct Line
    {
        RingDelay delay;
        GlideLength length;
        double baseSamples = 1.0;       // at size scale 1
        int whole = 1;                  // the settled length, for at()
        bool modulated = false;
        double oscSin = 0.0, oscCos = 1.0, oscStep = 0.0, oscSinStart = 0.0, oscCosStart = 1.0;
        float shelf = 0.0f;             // the one-pole's state
        float gLo = 0.0f, gHi = 0.0f, gLoStep = 0.0f, gHiStep = 0.0f;
        float gLoTarget = 0.0f, gHiTarget = 0.0f;
    };

    static bool parity(int x)
    {
        bool p = false;
        for (; x != 0; x &= x - 1) p = ! p;
        return p;
    }

    void applyLengths()
    {
        for (auto& line : lines) {
            double samples = line.baseSamples * sizeScale;
            // A modulated line is always read between samples. The others
            // settle on a whole number: an interpolator left in the loop is a
            // second, uncontrolled damping filter (8 kHz RT60 on Hall: integer
            // 1.75 s, Lagrange 1.68 s, linear 1.35 s).
            if (! line.modulated) samples = std::round(samples);
            line.length.set(std::max(samples, modDepth + 2.0));
            if (line.length.moving) gainsLive = true;
        }
    }

    // Each line's gains from its current length and the current decay time.
    void computeGains()
    {
        for (auto& line : lines) {
            const double gHi = rt60ToGain(line.length.current, fs, t60 * cfg.hfRatio);
            const double gMid = rt60ToGain(line.length.current, fs, t60);
            line.gHiTarget = static_cast<float>(gHi);
            line.gLoTarget = static_cast<float>(solveShelfLow(gHi, gMid, shelfRef));
        }
    }

    // While a length or the decay time moves, gains are recomputed every
    // kGainInterval samples and ramped linearly in between; one more ramp
    // after the movement stops, then they are set exactly and left alone.
    void retargetGains()
    {
        bool moving = t60Moving;
        for (const auto& line : lines) moving = moving || line.length.moving;

        computeGains();
        const bool ramp = moving || gainsSettling;
        for (auto& line : lines) {
            if (ramp) {
                line.gLoStep = (line.gLoTarget - line.gLo) / kGainInterval;
                line.gHiStep = (line.gHiTarget - line.gHi) / kGainInterval;
            } else {
                line.gLo = line.gLoTarget; line.gHi = line.gHiTarget;
                line.gLoStep = line.gHiStep = 0.0f;
            }
        }
        gainsLive = ramp;
        gainsSettling = moving;
        gainCountdown = ramp ? kGainInterval : 1;   // 1: the caller's decrement leaves 0, ready for the next move
    }

    // Input diffusion: four allpasses per channel, L and R lengths different.
    static constexpr double kDiffuserMsL[4] = { 4.771, 3.595, 12.73, 9.307 };
    static constexpr double kDiffuserMsR[4] = { 5.107, 3.881, 11.83, 9.911 };

    std::array<Line, kLines> lines;
    std::array<Allpass, 4> diffuserL, diffuserR;
    std::array<float, kLines> inL {}, inR {}, outL {}, outR {};

    FdnConfig cfg { 20.0f, 60.0f, 1, 0.5f, 5000.0f, 0.0f, 0.0f, 1.0f };
    ShelfReference shelfRef;
    double fs = 44100.0;
    double sizeScale = 1.0;
    double modDepth = 0.0;
    double lengthGlide = 1.0, decayGlide = 1.0;
    double t60 = 1.0, t60Target = 1.0;
    float shelfPole = 0.0f;
    int gainCountdown = 0;
    bool t60Moving = false;
    bool gainsLive = false;
    bool gainsSettling = false;
};

} // namespace osr
