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

    O-SimpleReverb - spring (three dispersive springs)
    Ouaricon Audio

    The low-chirp structure of Valimaki, Parker & Abel, "Parametric Spring
    Reverberation Effect", JAES 2010. One spring is a feedback loop holding a
    delay line, a cascade of stretched first-order allpasses
    (a + z^-K) / (1 + a z^-K), a 6th-order low-pass at the transition
    frequency, a 100 Hz high-pass and an inverting reflection. The cascade's
    group delay rises towards the transition frequency fC = fs / (2 K), so each
    echo arrives low frequencies first - the chirp - and every further trip
    round the loop stretches it again. The paper's separate high-frequency
    chirp path sits 30 dB under this one and is left out.

    What is not in the paper:

      - Decay time is a number in seconds. An echo at 1 kHz takes the loop
        delay plus the group delay there of the cascade and of the two
        filters; the reflection gain is 10^(-3 echo / T60). It is taken from
        the CURRENT loop length, so the tail time holds while SIZE moves it.
      - SIZE scales the echo time and glides. It moves the delay line only:
        the cascade is the spring's material and stays as it is.
      - Three springs of different echo time, stage count and transition
        frequency, as an amp tank has. L feeds the first, R the second, and
        (L + R) / sqrt 2 the third, which goes to both outputs.
      - The delay wobbles slowly on a sine from a fixed phase where the
        literature uses filtered noise: no Random, no clock, nothing taken
        from `this`, so two renders of the same input are the same file.

    K is a whole number of samples, so the transition frequency a spring gets
    is the nearest fs / (2 K) to the one it asks for: 4000 / 4800 / 4800 Hz at
    48 kHz, 4410 on all three at 44.1 kHz, 4364 / 4364 / 4800 Hz at 96 kHz.
    The decay time does not move with it (the echo time is computed from the K
    in use); the chirp's length does, a little.

    The output is NOT decorrelated: the third spring is in both channels
    (late-tail L/R correlation about 0.33 for a mono input, as v1.14.0's
    Spring had). A host folding to mono uses kMonoFold, not 0.7071.

    LOOP GAIN: the reflection gain is <= kGainCeiling (0.98). Everything else
    on the loop is an allpass, a Butterworth low-pass or high-pass (|H| <= 1),
    or a Lagrange read in its central interval (|H| <= 1).

  ==============================================================================
*/

#pragma once
#include "ReverbPrimitives.h"
#include <array>
#include <complex>

namespace osr
{

class SpringEngine
{
public:
    static constexpr int kSprings = 3;
    static constexpr double kMaxSizeScale = 4.0 / 3.0;   // of the echo times below
    static constexpr double kGainCeiling = 0.98;          // the loop-gain bound, see above
    static constexpr double kMidReferenceHz = 1000.0;
    static constexpr double kDecayGlideSeconds = 0.06;    // DECAY moves: the gains slew
    static constexpr int kGainInterval = 16;              // samples between recomputes while anything moves

    static constexpr float kAllpassCoefficient = 0.62f;   // the paper's a1
    static constexpr double kHighPassHz = 100.0;
    static constexpr double kWobbleMs = 0.15;             // peak excursion of the loop delay. A voicing.

    // The third spring's level in each output, and the fold a mono bus needs
    // for a mono input to come out as loud as one channel of the stereo
    // output: there the third spring is fed sqrt 2 and arrives twice.
    static constexpr float kSharedLevel = 0.5f;
    static constexpr float kMonoFold = 0.61237244f;       // sqrt((1 + 2 w^2) / (2 + 8 w^2)), w = kSharedLevel

    // Echo times are the Accutronics figures for a three-spring amp tank.
    struct SpringDef { double echoMs; double transitionHz; int stages; double wobbleHz; double wobblePhase; };
    static constexpr SpringDef kSpring[kSprings] = {
        { 33.0, 4300.0, 72, 0.71, 0.0 },
        { 37.0, 4550.0, 80, 0.93, 2.1 },
        { 41.0, 4800.0, 88, 1.19, 4.2 } };

    void prepare(double sampleRate)
    {
        fs = sampleRate;
        wobble = kWobbleMs * 0.001 * sampleRate;

        for (int i = 0; i < kSprings; ++i) {
            auto& s = springs[static_cast<size_t>(i)];
            const auto& def = kSpring[i];
            s.stretch = std::max(1, static_cast<int>(std::lround(sampleRate / (2.0 * def.transitionHz))));
            s.state.assign(static_cast<size_t>(def.stages * s.stretch), 0.0f);
            s.delay.prepare(static_cast<int>(std::ceil(def.echoMs * kMaxSizeScale * 0.001 * sampleRate + wobble)) + 2);

            // Just under the cascade's own transition frequency fs / (2 K),
            // above which its group delay comes back down
            const double lowPassHz = std::min(0.95 * sampleRate / (2.0 * s.stretch), def.transitionHz);
            s.lowPass[0].setLowPass(sampleRate, lowPassHz, 0.5176);    // 6th-order Butterworth
            s.lowPass[1].setLowPass(sampleRate, lowPassHz, 0.7071);
            s.lowPass[2].setLowPass(sampleRate, lowPassHz, 1.9319);
            s.highPass.setHighPass(sampleRate, kHighPassHz, 0.7071);

            s.oscStep = 2.0 * std::sin(kPi * def.wobbleHz / sampleRate);
        }

        lengthGlide = slewCoefficient(0.5 * kSizeGlideSeconds, sampleRate);
        decayGlide = slewCoefficient(kDecayGlideSeconds, sampleRate);

        applyVoicing();
        reset();
    }

    // Echo-time scale, 1 = the echo times above. The loop delays glide there.
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

    // Takes the allpass cascade or the low-pass out of the loop, for the gates
    // that show the chirp and the band limit are theirs. Not under signal:
    // follow with reset().
    void setVoicing(bool dispersion, bool bandLimit)
    {
        if (dispersion == dispersive && bandLimit == bandLimited) return;
        dispersive = dispersion;
        bandLimited = bandLimit;
        applyVoicing();
    }

    // Lands every glide on its target without clearing anything.
    void land()
    {
        for (auto& s : springs) s.length.land();
        t60 = t60Target;
        t60Moving = false;
        computeTargets();
        for (auto& s : springs) {
            s.gain = s.gainTarget;
            s.gainStep = 0.0f;
        }
        gainsLive = false;
        gainsSettling = false;
        gainCountdown = 0;
    }

    // Clears the springs and lands the glides.
    void reset()
    {
        for (int i = 0; i < kSprings; ++i) {
            auto& s = springs[static_cast<size_t>(i)];
            s.delay.reset();
            std::fill(s.state.begin(), s.state.end(), 0.0f);
            s.phase = 0;
            for (auto& lp : s.lowPass) lp.reset();
            s.highPass.reset();
            s.oscSin = std::sin(kSpring[i].wobblePhase);
            s.oscCos = std::cos(kSpring[i].wobblePhase);
        }
        land();
    }

    void process(float l, float r, float& yL, float& yR)
    {
        if (t60Moving) {
            t60 += (t60Target - t60) * decayGlide;
            if (std::abs(t60Target - t60) < 1.0e-4 * t60Target) { t60 = t60Target; t60Moving = false; }
        }
        if (gainsLive) {
            if (gainCountdown == 0) retarget();
            --gainCountdown;
        }

        const float in[kSprings] = { l, r, 0.70710678f * (l + r) };
        float out[kSprings];
        for (int i = 0; i < kSprings; ++i) {
            auto& s = springs[static_cast<size_t>(i)];
            if (s.length.moving) s.length.tick(lengthGlide);
            s.gain += s.gainStep;

            s.oscSin += s.oscStep * s.oscCos;     // coupled form: bounded, no sin() per sample
            s.oscCos -= s.oscStep * s.oscSin;

            // Read first, push after (see RingDelay). Always between samples:
            // the wobble never stops. The kernel's roll-off starts two octaves
            // above the loop's own low-pass.
            float y = s.delay.readLagrange(s.length.current + wobble * s.oscSin);

            // (a + z^-K) / (1 + a z^-K), transposed direct form. Each stage
            // holds K samples; the K-th of every stage sits side by side.
            float* state = s.state.data() + s.phase * kSpring[i].stages;
            for (int m = 0; m < s.activeStages; ++m) {
                const float v = kAllpassCoefficient * y + state[m];
                state[m] = y - kAllpassCoefficient * v;
                y = v;
            }
            if (++s.phase == s.stretch) s.phase = 0;

            if (bandLimited) y = s.lowPass[2].process(s.lowPass[1].process(s.lowPass[0].process(y)));
            y = s.highPass.process(y);

            s.delay.push(in[i] - s.gain * y);     // inverting reflection; |gain| <= kGainCeiling
            out[i] = y;
        }

        yL = out[0] + kSharedLevel * out[2];
        yR = out[1] + kSharedLevel * out[2];
    }

    // Where spring i is heading: its echo time at 1 kHz, and what the decay
    // time makes of its reflection gain. transitionHz is the fs / (2 K) in use.
    double echoSeconds(int i) const
    {
        const auto& s = springs[static_cast<size_t>(i)];
        return (s.length.target + s.fixedDelay) / fs;
    }
    float reflectionGain(int i) const { return springs[static_cast<size_t>(i)].gainTarget; }
    double transitionHz(int i) const { return fs / (2.0 * springs[static_cast<size_t>(i)].stretch); }

private:
    // RBJ biquad, transposed direct form II.
    struct Biquad
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f, z1 = 0.0f, z2 = 0.0f;

        void reset() { z1 = z2 = 0.0f; }

        float process(float x)
        {
            const float y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }

        void setLowPass(double sampleRate, double hz, double q)
        {
            const double w = 2.0 * kPi * hz / sampleRate, alpha = std::sin(w) / (2.0 * q), c = std::cos(w), a0 = 1.0 + alpha;
            b0 = static_cast<float>((1.0 - c) / 2.0 / a0); b1 = static_cast<float>((1.0 - c) / a0); b2 = b0;
            a1 = static_cast<float>(-2.0 * c / a0); a2 = static_cast<float>((1.0 - alpha) / a0);
        }

        void setHighPass(double sampleRate, double hz, double q)
        {
            const double w = 2.0 * kPi * hz / sampleRate, alpha = std::sin(w) / (2.0 * q), c = std::cos(w), a0 = 1.0 + alpha;
            b0 = static_cast<float>((1.0 + c) / 2.0 / a0); b1 = static_cast<float>(-(1.0 + c) / a0); b2 = b0;
            a1 = static_cast<float>(-2.0 * c / a0); a2 = static_cast<float>((1.0 - alpha) / a0);
        }

        std::complex<double> response(double hz, double sampleRate) const
        {
            const auto z1inv = std::polar(1.0, -2.0 * kPi * hz / sampleRate), z2inv = z1inv * z1inv;
            return (static_cast<double>(b0) + static_cast<double>(b1) * z1inv + static_cast<double>(b2) * z2inv)
                   / (1.0 + static_cast<double>(a1) * z1inv + static_cast<double>(a2) * z2inv);
        }
    };

    struct Spring
    {
        RingDelay delay;
        GlideLength length;             // the delay line; the echo time is this plus fixedDelay
        std::vector<float> state;       // stages x stretch, see process()
        std::array<Biquad, 3> lowPass;
        Biquad highPass;
        int stretch = 5;                // K
        int activeStages = 0;
        int phase = 0;                  // which of a stage's K samples is current
        double fixedDelay = 0.0;        // group delay at 1 kHz of the cascade and the filters, in samples
        double oscSin = 0.0, oscCos = 1.0, oscStep = 0.0;
        float gain = 0.0f, gainStep = 0.0f, gainTarget = 0.0f;
    };

    // What the loop holds besides the delay line, and the group delay of it at
    // 1 kHz: one stretched allpass delays by K (1 - a^2) / (1 + 2 a cos(w K) + a^2)
    // samples; the filters' is read off their phase.
    void applyVoicing()
    {
        const double a = kAllpassCoefficient;
        for (int i = 0; i < kSprings; ++i) {
            auto& s = springs[static_cast<size_t>(i)];
            s.activeStages = dispersive ? kSpring[i].stages : 0;

            const double w = 2.0 * kPi * kMidReferenceHz / fs * s.stretch;
            const double cascade = s.activeStages * s.stretch * (1.0 - a * a) / (1.0 + 2.0 * a * std::cos(w) + a * a);

            const double halfSpan = 5.0;
            auto filtersAt = [&](double hz) {
                auto h = s.highPass.response(hz, fs);
                if (bandLimited)
                    for (const auto& lp : s.lowPass) h *= lp.response(hz, fs);
                return h;
            };
            const double phaseStep = std::arg(filtersAt(kMidReferenceHz + halfSpan) / filtersAt(kMidReferenceHz - halfSpan));
            s.fixedDelay = cascade - phaseStep / (2.0 * kPi * 2.0 * halfSpan / fs);
        }
        applyLengths();
    }

    void applyLengths()
    {
        const double shortest = wobble + 3.0;       // a wobbling read stays 2 clear of the write head
        for (int i = 0; i < kSprings; ++i) {
            auto& s = springs[static_cast<size_t>(i)];
            s.length.set(std::max(shortest, kSpring[i].echoMs * sizeScale * 0.001 * fs - s.fixedDelay));
            if (s.length.moving) gainsLive = true;
        }
    }

    // Each spring's reflection gain from its current echo time and the current
    // decay time.
    void computeTargets()
    {
        for (auto& s : springs)
            s.gainTarget = static_cast<float>(std::min(kGainCeiling, rt60ToGain(s.length.current + s.fixedDelay, fs, t60)));
    }

    // As FdnEngine::retargetGains: while a length or the decay time moves, the
    // gains are recomputed every kGainInterval samples and ramped linearly in
    // between; one more ramp after the movement stops, then they are set
    // exactly and left alone.
    void retarget()
    {
        bool moving = t60Moving;
        for (const auto& s : springs) moving = moving || s.length.moving;

        computeTargets();
        const bool ramp = moving || gainsSettling;
        for (auto& s : springs) {
            if (ramp) {
                s.gainStep = (s.gainTarget - s.gain) / kGainInterval;
            } else {
                s.gain = s.gainTarget;
                s.gainStep = 0.0f;
            }
        }
        gainsLive = ramp;
        gainsSettling = moving;
        gainCountdown = ramp ? kGainInterval : 1;   // 1: the caller's decrement leaves 0, ready for the next move
    }

    std::array<Spring, kSprings> springs;

    double fs = 44100.0;
    double sizeScale = 1.0;
    double wobble = 0.0;
    double lengthGlide = 1.0, decayGlide = 1.0;
    double t60 = 2.5, t60Target = 2.5;
    int gainCountdown = 0;
    bool dispersive = true;
    bool bandLimited = true;
    bool t60Moving = false;
    bool gainsLive = false;
    bool gainsSettling = false;
};

} // namespace osr
