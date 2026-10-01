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

    O-SimpleReverb - plate (Dattorro's figure-8 tank)
    Ouaricon Audio

    Dattorro, "Effect Design Part 1", JAES 1997, Fig. 1 and Tables 1-2: two
    halves, each a modulated allpass, a delay, a damping low-pass, a second
    allpass and a second delay, each half feeding the other. Every length is
    the paper's, at its 29761 Hz design rate, times fs / 29761 and the size
    scale. What is not in the paper:

      - Decay time is a number in seconds. The paper sets `decay` by ear. One
        trip round the figure-8 passes `decay` four times and the eight loop
        elements once, so decay^4 is the gain of a loop that long - see
        computeTargets(). It is taken from the CURRENT lengths, so the tail
        time holds while SIZE moves them.
      - Stereo in: L feeds one half and R the other, through separate input
        diffusers. The paper sums the input to mono.
      - Shimmer inside the loop: a share of each cross-feed goes through the
        octave-up shifter, so the octave is absent at the onset and builds up
        over the tail, each trip adding a little more.
      - SIZE glides (every length and every output tap), and the modulated
        allpasses are read with a 3rd-order Lagrange kernel, not linearly.
      - No input bandwidth filter (the paper's 0.9995 is a pass-through), and
        the damping is a cutoff in Hz, so it is the same filter at any rate.

    Echo density is this tank's weak point: it deliberately undershoots
    Schroeder's criterion. Normalised density 100 ms in is 0.60 at the paper's
    own size against 0.84 at x0.55, which is why the host runs it at
    x0.40..x0.80 and never at x1.

    LOOP GAIN: `decay` <= kDecayCeiling (0.97). Everything else on the loop is
    an allpass, a delay, a low-pass with unity gain at DC, a Lagrange read in
    its central interval (|H| <= 1), or the shimmer mix. That mix is CONVEX,
    (1 - s) x + s shift(x): the shifter's two windows sum to 1 and it reads
    each input sample with total weight 1, so it adds no energy, and a convex
    mix of it with the identity adds none either. An additive mix,
    x + s shift(x), would bound the loop at decay (1 + s) instead.

    The modulation starts from a fixed phase: no Random, no clock, nothing
    taken from `this`.

  ==============================================================================
*/

#pragma once
#include "ReverbPrimitives.h"
#include "../ModulationFx.h"
#include <array>

namespace osr
{

class PlateEngine
{
public:
    static constexpr double kDesignRate = 29761.0;
    static constexpr double kMaxSizeScale = 1.0;          // the paper's own size; the host stays under it
    static constexpr double kDecayCeiling = 0.97;         // the loop-gain bound, see above
    static constexpr double kMidReferenceHz = 1000.0;
    static constexpr double kDecayGlideSeconds = 0.06;    // DECAY moves: the coefficient slews
    static constexpr int kGainInterval = 16;              // samples between recomputes while anything moves

    // Measured: the tank decays more slowly than its loop length says, because
    // the allpasses hold energy back. The same figure at 44.1, 48 and 96 kHz.
    static constexpr double kLoopCorrection = 1.055;

    // Share of each cross-feed that goes through the shifter, per second of
    // loop time: the octave then builds at the same rate per second of tail at
    // every SIZE. 0.121 is a share of 0.05 at SIZE 50 (a 0.41 s loop). A
    // voicing - set by ear.
    static constexpr double kShimmerPerLoopSecond = 0.121;
    // The shifter takes energy out of the mid band (it leaves upward), and the
    // decay time is measured there. This much of the share is given back
    // through `decay`; without it a share of 0.08 shortens a 5 s tail by 10-19 %.
    static constexpr double kShimmerDrain = 0.9;
    static constexpr double kShimmerLowPassHz = 4500.0;   // ahead of the shifter: what it doubles stays under Nyquist

    static constexpr double kDampingHz = 10000.0;         // in-loop low-pass. A voicing.
    static constexpr double kExcursionMs = 0.27;          // the paper's 8 samples peak at 29761 Hz
    static constexpr double kModulationHz = 1.0;

    void prepare(double sampleRate)
    {
        fs = sampleRate;
        rateScale = sampleRate / kDesignRate;
        excursion = kExcursionMs * 0.001 * sampleRate;

        for (int i = 0; i < kLoopLines; ++i)
            lines[static_cast<size_t>(i)].delay.prepare(static_cast<int>(std::ceil(kLoopRef[i] * rateScale * kMaxSizeScale + excursion)) + 2);

        // Input diffusion does not scale with SIZE
        for (int k = 0; k < 4; ++k) {
            const float g = k < 2 ? 0.75f : 0.625f;
            const int lengthL = static_cast<int>(std::lround(kDiffuserRefL[k] * rateScale));
            const int lengthR = static_cast<int>(std::lround(kDiffuserRefR[k] * rateScale));
            diffuserL[static_cast<size_t>(k)].prepare(lengthL);
            diffuserL[static_cast<size_t>(k)].set(lengthL, g);
            diffuserR[static_cast<size_t>(k)].prepare(lengthR);
            diffuserR[static_cast<size_t>(k)].set(lengthR, g);
        }

        dampA.setCutoff(kDampingHz, sampleRate);
        dampB.setCutoff(kDampingHz, sampleRate);
        const auto ref = shelfReference(dampA.a, kMidReferenceHz, sampleRate);
        dampingMid = std::sqrt(ref.re * ref.re + ref.im * ref.im);

        shimmerLpA.setCutoff(kShimmerLowPassHz, sampleRate);
        shimmerLpB.setCutoff(kShimmerLowPassHz, sampleRate);
        shifterA.prepare(sampleRate);
        shifterB.prepare(sampleRate);

        oscStep = 2.0 * std::sin(kPi * kModulationHz / sampleRate);
        lengthGlide = slewCoefficient(0.5 * kSizeGlideSeconds, sampleRate);
        decayGlide = slewCoefficient(kDecayGlideSeconds, sampleRate);

        applyLengths();
        reset();
    }

    // Length scale, 1 = the paper's lengths. Lengths and output taps glide there.
    void setSize(double scale)
    {
        scale = std::min(scale, kMaxSizeScale);
        if (sameValue(scale, sizeScale)) return;
        sizeScale = scale;
        applyLengths();
    }

    // Mid-band decay time in seconds. The coefficients slew there.
    void setT60(double seconds)
    {
        seconds = std::max(0.05, seconds);
        if (sameValue(seconds, t60Target)) return;
        t60Target = seconds;
        t60Moving = true;
        gainsLive = true;
    }

    // Shifter share per second of loop time; 0 takes the shifter out of the
    // loop. Not under signal: follow with reset() or land().
    void setShimmer(double sharePerLoopSecond) { shimmerRate = std::max(0.0, sharePerLoopSecond); }

    // Lands every glide on its target without clearing anything.
    void land()
    {
        for (auto& line : lines) landOne(line);
        for (auto* taps : { &tapL, &tapR })
            for (auto& tap : *taps) landOne(tap);
        t60 = t60Target;
        t60Moving = false;
        computeTargets();
        decay = decayTarget; diffusion2 = diffusion2Target; share = shareTarget;
        decayStep = diffusion2Step = shareStep = 0.0f;
        gainsLive = false;
        gainsSettling = false;
        gainCountdown = 0;
    }

    // Clears the tank and lands the glides.
    void reset()
    {
        for (auto& line : lines) line.delay.reset();
        for (auto* bank : { &diffuserL, &diffuserR })
            for (auto& ap : *bank) ap.reset();
        dampA.reset(); dampB.reset();
        shimmerLpA.reset(); shimmerLpB.reset();
        shifterA.reset(); shifterB.reset();
        oscSin = 0.0; oscCos = 1.0;
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
            if (gainCountdown == 0) retarget();
            --gainCountdown;
        }
        decay += decayStep;
        diffusion2 += diffusion2Step;
        share += shareStep;

        for (auto& line : lines) tickOne(line);
        for (auto* taps : { &tapL, &tapR })
            for (auto& tap : *taps) tickOne(tap);

        oscSin += oscStep * oscCos;     // coupled form: bounded, no sin() per sample
        oscCos -= oscStep * oscSin;

        // Every read comes before any push (see RingDelay): a line of length D
        // is exactly D samples of delay.
        const float midA = readLine(dA1), endA = readLine(dA2);
        const float midB = readLine(dB1), endB = readLine(dB2);

        float sumL = 0.0f, sumR = 0.0f;
        for (int k = 0; k < kTapsPerSide; ++k) {
            sumL += kTapL[k].sign * readTap(tapL[static_cast<size_t>(k)], kTapL[k].line);
            sumR += kTapR[k].sign * readTap(tapR[static_cast<size_t>(k)], kTapR[k].line);
        }

        // Half A, fed by L and by the end of half B; half B the other way round.
        // `decay` is applied twice in each half: four times per trip.
        const float a = modulatedAllpass(lines[ap1A], l + decay * cross(endB, shifterB, shimmerLpB),
                                         excursion * oscSin);
        lines[dA1].delay.push(a);
        lines[dA2].delay.push(allpass(lines[ap2A], decay * dampA.lowPass(midA), diffusion2));

        const float b = modulatedAllpass(lines[ap1B], r + decay * cross(endA, shifterA, shimmerLpA),
                                         excursion * oscCos);
        lines[dB1].delay.push(b);
        lines[dB2].delay.push(allpass(lines[ap2B], decay * dampB.lowPass(midB), diffusion2));

        yL = kOutputGain * sumL;
        yR = kOutputGain * sumR;
    }

    // Where the loop is heading: its length in seconds (the eight elements),
    // and what the decay time and that length make of the coefficients.
    double loopSeconds() const
    {
        double samples = 0.0;
        for (const auto& line : lines) samples += line.length.target;
        return samples / fs;
    }
    float decayCoefficient() const { return decayTarget; }
    float shimmerShare() const { return shareTarget; }

private:
    // The eight loop elements, in the order the signal meets them.
    enum LineId { ap1A, dA1, ap2A, dA2, ap1B, dB1, ap2B, dB2, kLoopLines };
    static constexpr double kLoopRef[kLoopLines] = { 672.0, 4453.0, 1800.0, 3720.0, 908.0, 4217.0, 2656.0, 3163.0 };
    static constexpr double kDiffuserRefL[4] = { 142.0, 107.0, 379.0, 277.0 };   // the paper's
    static constexpr double kDiffuserRefR[4] = { 151.0, 113.0, 367.0, 283.0 };   // the other channel's, a few percent off

    // The paper's fourteen output taps (Table 2), seven a side, each side
    // reading mostly from the OTHER half of the tank.
    struct TapDef { LineId line; double ref; float sign; };
    static constexpr int kTapsPerSide = 7;
    static constexpr TapDef kTapL[kTapsPerSide] = {
        { dB1, 266.0, 1.0f }, { dB1, 2974.0, 1.0f }, { ap2B, 1913.0, -1.0f }, { dB2, 1996.0, 1.0f },
        { dA1, 1990.0, -1.0f }, { ap2A, 187.0, -1.0f }, { dA2, 1066.0, -1.0f } };
    static constexpr TapDef kTapR[kTapsPerSide] = {
        { dA1, 353.0, 1.0f }, { dA1, 3627.0, 1.0f }, { ap2A, 1228.0, -1.0f }, { dA2, 2673.0, 1.0f },
        { dB1, 2111.0, -1.0f }, { ap2B, 335.0, -1.0f }, { dB2, 121.0, -1.0f } };
    static constexpr float kOutputGain = 0.6f;
    static constexpr float kDecayDiffusion1 = -0.70f;   // the paper's 0.70, sign inverted as in its Fig. 1

    struct Position
    {
        GlideLength length;
        int whole = 2;      // the settled length, for at()
    };
    struct Line : Position { RingDelay delay; };
    using Taps = std::array<Position, kTapsPerSide>;

    static void landOne(Position& p)
    {
        p.length.land();
        p.whole = static_cast<int>(p.length.current);
    }

    void tickOne(Position& p) const
    {
        if (! p.length.moving) return;
        p.length.tick(lengthGlide);
        if (! p.length.moving) p.whole = static_cast<int>(p.length.current);
    }

    float readLine(LineId id) const
    {
        const auto& line = lines[id];
        return line.length.moving ? line.delay.readLagrange(line.length.current) : line.delay.at(line.whole);
    }

    float readTap(const Position& tap, LineId id) const
    {
        const auto& delay = lines[id].delay;
        return tap.length.moving ? delay.readLagrange(tap.length.current) : delay.at(tap.whole);
    }

    // H(z) = (-g + z^-D) / (1 - g z^-D), as osr::Allpass, on a line whose
    // length can glide.
    float allpass(Line& line, float x, float g) const
    {
        const float delayed = line.length.moving ? line.delay.readLagrange(line.length.current) : line.delay.at(line.whole);
        const float v = x + g * delayed;
        line.delay.push(v);
        return delayed - g * v;
    }

    float modulatedAllpass(Line& line, float x, double offset) const
    {
        const float delayed = line.delay.readLagrange(line.length.current + offset);
        const float v = x + kDecayDiffusion1 * delayed;
        line.delay.push(v);
        return delayed - kDecayDiffusion1 * v;
    }

    // One cross-feed: a convex mix of the signal and its octave. Gain <= 1.
    float cross(float x, OctaveUpShifter& shifter, OnePole& lowPass) const
    {
        if (shimmerRate <= 0.0) return x;
        return x + share * (shifter.process(lowPass.lowPass(x)) - x);
    }

    void applyLengths()
    {
        const double scale = rateScale * sizeScale;
        const double shortest = excursion + 3.0;    // a modulated read stays 2 clear of the write head
        for (int i = 0; i < kLoopLines; ++i) {
            auto& line = lines[static_cast<size_t>(i)];
            line.length.set(std::max(shortest, std::round(kLoopRef[i] * scale)));
            if (line.length.moving) gainsLive = true;
        }
        for (int k = 0; k < kTapsPerSide; ++k) {
            tapL[static_cast<size_t>(k)].length.set(std::max(2.0, std::round(kTapL[k].ref * scale)));
            tapR[static_cast<size_t>(k)].length.set(std::max(2.0, std::round(kTapR[k].ref * scale)));
        }
    }

    // The coefficients from the loop's current length and the current decay
    // time. One trip round the figure-8 is the eight elements (an allpass
    // delays by its length on average), and on it the signal meets `decay`
    // four times, the damping low-pass twice and the shifter twice:
    //
    //   decay^4 x damping(1 kHz)^2 x (1 - drain x share) = 10^(-3 loop / T60)
    void computeTargets()
    {
        double samples = 0.0;
        for (const auto& line : lines) samples += line.length.current;

        const double s = shimmerRate > 0.0 ? std::min(1.0, shimmerRate * samples / fs) : 0.0;
        const double perTrip = rt60ToGain(samples * kLoopCorrection, fs, t60);
        const double d = std::min(kDecayCeiling,
                                  std::sqrt(std::sqrt(perTrip)) / std::sqrt(dampingMid * (1.0 - kShimmerDrain * s)));
        decayTarget = static_cast<float>(d);
        // the paper: decay diffusion 2 = decay + 0.15, floor 0.25, ceiling 0.50
        diffusion2Target = static_cast<float>(std::max(0.25, std::min(0.50, d + 0.15)));
        shareTarget = static_cast<float>(s);
    }

    // As FdnEngine::retargetGains: while a length or the decay time moves, the
    // coefficients are recomputed every kGainInterval samples and ramped
    // linearly in between; one more ramp after the movement stops, then they
    // are set exactly and left alone.
    void retarget()
    {
        bool moving = t60Moving;
        for (const auto& line : lines) moving = moving || line.length.moving;

        computeTargets();
        const bool ramp = moving || gainsSettling;
        if (ramp) {
            decayStep = (decayTarget - decay) / kGainInterval;
            diffusion2Step = (diffusion2Target - diffusion2) / kGainInterval;
            shareStep = (shareTarget - share) / kGainInterval;
        } else {
            decay = decayTarget; diffusion2 = diffusion2Target; share = shareTarget;
            decayStep = diffusion2Step = shareStep = 0.0f;
        }
        gainsLive = ramp;
        gainsSettling = moving;
        gainCountdown = ramp ? kGainInterval : 1;   // 1: the caller's decrement leaves 0, ready for the next move
    }

    std::array<Line, kLoopLines> lines;
    Taps tapL, tapR;
    std::array<Allpass, 4> diffuserL, diffuserR;
    OnePole dampA, dampB, shimmerLpA, shimmerLpB;
    OctaveUpShifter shifterA, shifterB;

    double fs = 44100.0;
    double rateScale = 44100.0 / kDesignRate;
    double sizeScale = 0.57;
    double excursion = 0.0;
    double dampingMid = 1.0;            // the damping low-pass's gain at 1 kHz
    double shimmerRate = kShimmerPerLoopSecond;
    double oscSin = 0.0, oscCos = 1.0, oscStep = 0.0;
    double lengthGlide = 1.0, decayGlide = 1.0;
    double t60 = 2.5, t60Target = 2.5;
    float decay = 0.5f, diffusion2 = 0.5f, share = 0.0f;
    float decayStep = 0.0f, diffusion2Step = 0.0f, shareStep = 0.0f;
    float decayTarget = 0.5f, diffusion2Target = 0.5f, shareTarget = 0.0f;
    int gainCountdown = 0;
    bool t60Moving = false;
    bool gainsLive = false;
    bool gainsSettling = false;
};

} // namespace osr
