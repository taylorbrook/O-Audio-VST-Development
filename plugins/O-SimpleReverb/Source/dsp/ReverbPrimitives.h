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

    O-SimpleReverb - reverb building blocks
    Ouaricon Audio

    What the engines in this directory are made of. Header-only and free of
    JUCE, so tests/render-check drives each piece directly. Audio is float;
    anything that is slewed over many samples (a length, a decay time) is
    double, because a float one-pole stalls short of its target once the
    remaining distance times the coefficient falls under half an ulp - about
    100 samples away on a 50 000-sample line at 192 kHz.

    Nothing here allocates outside prepare().

  ==============================================================================
*/

#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

namespace osr
{

constexpr double kPi = 3.14159265358979323846;

// "Has this value changed at all" - an exact comparison, and meant as one.
inline bool sameValue(double a, double b) { return ! (a < b) && ! (a > b); }

// Power-of-two ring delay.
//
// at(d) is the sample pushed d pushes ago: at(1) is the last one written.
// Every user READS FIRST AND PUSHES AFTER, once per sample, so a read at(D)
// realises exactly D samples of delay (y[n] = x[n - D]). Pushing first would
// make the same call D - 1; nothing in this directory does.
class RingDelay
{
public:
    // maxDelay: the longest delay any read will ask for, readLagrange included.
    void prepare(int maxDelay)
    {
        int n = 1;
        while (n < maxDelay + 4) n <<= 1;   // readLagrange looks 2 past its integer part
        buffer.assign(static_cast<size_t>(n), 0.0f);
        mask = n - 1;
        writePos = 0;
        longest = maxDelay;
    }

    void reset() { std::fill(buffer.begin(), buffer.end(), 0.0f); }

    void push(float x)
    {
        buffer[static_cast<size_t>(writePos)] = x;
        writePos = (writePos + 1) & mask;
    }

    // d >= 1
    float at(int d) const { return buffer[static_cast<size_t>((writePos - d) & mask)]; }

    // 3rd-order Lagrange at a fractional delay d >= 2. The fractional part
    // sits in the CENTRAL interval of the four points, where the interpolator's
    // magnitude response never exceeds 1 - it can sit inside a feedback loop
    // without adding gain. It still rolls off the top octave, which is why
    // lines that are not being modulated or glided are read with at().
    float readLagrange(double d) const
    {
        const int i = static_cast<int>(d);
        const float f = static_cast<float>(d - i);
        const float ym = at(i - 1), y0 = at(i), y1 = at(i + 1), y2 = at(i + 2);
        const float c0 = -f * (f - 1.0f) * (f - 2.0f) * (1.0f / 6.0f);
        const float c1 = (f + 1.0f) * (f - 1.0f) * (f - 2.0f) * 0.5f;
        const float c2 = -(f + 1.0f) * f * (f - 2.0f) * 0.5f;
        const float c3 = (f + 1.0f) * f * (f - 1.0f) * (1.0f / 6.0f);
        return c0 * ym + c1 * y0 + c2 * y1 + c3 * y2;
    }

    int maxDelay() const { return longest; }

private:
    std::vector<float> buffer;
    int mask = 0;
    int writePos = 0;
    int longest = 0;
};

// Schroeder allpass, H(z) = (-g + z^-D) / (1 - g z^-D), |H| = 1.
class Allpass
{
public:
    void prepare(int maxLength) { line.prepare(std::max(1, maxLength)); }
    void reset() { line.reset(); }

    // Not under signal: the length jumps.
    void set(int lengthSamples, float coefficient)
    {
        length = std::max(1, std::min(lengthSamples, line.maxDelay()));
        g = coefficient;
    }

    float process(float x)
    {
        const float delayed = line.at(length);
        const float v = x + g * delayed;
        line.push(v);
        return delayed - g * v;
    }

    int getLength() const { return length; }

private:
    RingDelay line;
    int length = 1;
    float g = 0.5f;
};

// One-pole low-pass, unity gain at DC; highPass() is its complement.
struct OnePole
{
    float a = 0.0f;   // pole
    float z = 0.0f;

    void setCutoff(double hz, double sampleRate) { a = static_cast<float>(std::exp(-2.0 * kPi * hz / sampleRate)); }
    void reset() { z = 0.0f; }
    float lowPass(float x)  { z = x + a * (z - x); return z; }
    float highPass(float x) { return x - lowPass(x); }
};

// The gain a signal must take on each trip round a loop of lengthSamples for
// the loop to decay 60 dB in t60 seconds (Jot): 10^(-3 L / (fs T60)).
inline double rt60ToGain(double lengthSamples, double sampleRate, double t60)
{
    return std::pow(10.0, -3.0 * lengthSamples / (sampleRate * t60));
}

// Its inverse: the decay time a loop of lengthSamples has at per-trip gain g.
inline double gainToRt60(double g, double lengthSamples, double sampleRate)
{
    return -3.0 * lengthSamples / (sampleRate * std::log10(g));
}

// A loop's two-band absorption is y = gHi x + (gLo - gHi) LP(x), LP being a
// OnePole: gain gLo at DC, falling towards gHi above the crossover, and
// |H| <= gLo everywhere. The decay time is wanted at 1 kHz, not at DC, and the
// shelf has already started down by then (Hall read -5.8 % with gLo set from
// the target directly). ShelfReference is LP's complex response at the
// reference frequency; solveShelfLow returns the gLo that puts |H| exactly on
// gTarget there. Closed form: |gHi + d P|^2 = gTarget^2 is a quadratic in d.
struct ShelfReference { double re = 1.0, im = 0.0; };

inline ShelfReference shelfReference(double pole, double referenceHz, double sampleRate)
{
    const double w = 2.0 * kPi * referenceHz / sampleRate;
    const double dr = 1.0 - pole * std::cos(w), di = pole * std::sin(w), m = dr * dr + di * di;
    return { (1.0 - pole) * dr / m, -(1.0 - pole) * di / m };
}

// gTarget >= gHi. The ceiling keeps the loop gain under 1 whatever is asked.
inline double solveShelfLow(double gHi, double gTarget, ShelfReference p)
{
    const double p2 = p.re * p.re + p.im * p.im;
    const double d = (-gHi * p.re + std::sqrt(gHi * gHi * p.re * p.re - p2 * (gHi * gHi - gTarget * gTarget))) / p2;
    return std::min(0.9999, gHi + d);
}

// Coefficient of a one-pole slew with the given time constant.
inline double slewCoefficient(double seconds, double sampleRate)
{
    return 1.0 - std::exp(-1.0 / (seconds * sampleRate));
}

// SIZE moves glide: every length that SIZE scales slews towards its target, so
// a ringing tail bends in pitch instead of clicking. Two one-poles in series,
// half of this each. One pole alone starts moving at full speed - the pitch
// JUMPS at the move, a kink in the waveform that measured -40 dB above 3 kHz on
// Hall, 20 dB over a smoothed WET move. Through two, the length's velocity
// starts from zero and the pitch bends in.
constexpr double kSizeGlideSeconds = 0.25;

// A delay length in samples that glides to its target and LANDS on it: within
// a thousandth of a sample it snaps, so a line that is not modulated ends up on
// the integer it was given and can go back to a plain at() read.
// tick() takes slewCoefficient(kSizeGlideSeconds / 2, fs).
struct GlideLength
{
    double current = 1.0;
    double lead = 1.0;      // the first pole; `current` follows it
    double target = 1.0;
    bool moving = false;

    void set(double samples) { target = samples; moving = ! (sameValue(current, target) && sameValue(lead, target)); }
    void land() { current = lead = target; moving = false; }

    void tick(double coefficient)
    {
        lead += (target - lead) * coefficient;
        current += (lead - current) * coefficient;
        if (std::abs(target - current) < 1.0e-3 && std::abs(target - lead) < 1.0e-3) land();
    }
};

} // namespace osr
