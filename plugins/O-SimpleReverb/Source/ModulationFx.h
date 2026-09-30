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

    O-SimpleReverb - pre-reverb modulation (v1.13.0)
    Ouaricon Audio

    Two single-channel units the pre-reverb chain runs per channel. They
    replace the pre-1.13.0 stand-ins: a +/-3 % amplitude wobble called
    "flutter" (+/-0.26 dB, inaudible) and a 1.5 kHz ring modulator called
    "shimmer" (inharmonic sidebands, not an octave). Kept in their own header
    so tests/render-check can measure them directly.

  ==============================================================================
*/

#pragma once
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <vector>

// Pitch flutter: a short delay swept by a sine. The pitch ratio is 1 - d'(t),
// so a sweep of depth D samples at rate f peaks at a ratio deviation of
// 2*pi*f*D/fs. setModulation() inverts that from a depth in CENTS, so every
// type states its flutter as what it sounds like, not as milliseconds.
class FlutterDelay
{
public:
    // maxDepthMs: the deepest sweep any caller will set (sizes the line)
    void prepare(double sampleRate, float maxDepthMs)
    {
        sr = sampleRate;
        const int capacity = static_cast<int>(std::ceil(2.0 * maxDepthMs * 0.001 * sampleRate)) + 8;
        line.setMaximumDelayInSamples(capacity);
        line.prepare({ sampleRate, 1, 1 });
        reset();
    }

    void reset() { line.reset(); }

    // Not while signal is flowing: the centre delay moves with the depth.
    // The pre-reverb chain calls this only at its TYPE-duck zero.
    void setModulation(float rateHz, float cents)
    {
        if (rateHz <= 0.0f || cents <= 0.0f) { depth = 0.0f; centre = 2.0f; return; }
        const double ratio = std::pow(2.0, cents / 1200.0) - 1.0;
        depth = static_cast<float>(ratio * sr / (juce::MathConstants<double>::twoPi * rateHz));
        centre = depth + 2.0f;   // Lagrange needs a little room below the sweep
    }

    // lfo: the shared sine, -1..1
    float process(float x, float lfo)
    {
        line.pushSample(0, x);
        return line.popSample(0, centre + depth * lfo);
    }

    float getDepthSamples() const { return depth; }

    // Maximum pitch deviation of a setModulation() pair, in cents, as a sample-
    // rate-free check value.
    static float centsFor(float rateHz, float depthSamples, double sampleRate)
    {
        const double ratio = juce::MathConstants<double>::twoPi * rateHz * depthSamples / sampleRate;
        return static_cast<float>(1200.0 * std::log2(1.0 + ratio));
    }

private:
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> line;
    double sr = 44100.0;
    float depth = 0.0f;
    float centre = 2.0f;
};

// Octave-up shimmer: two read taps sweep a grain of W samples from delay W down
// to 0 at one sample per sample, so each plays at 2x. They run half a grain
// apart under sin^2 / cos^2 windows, which sum to exactly 1 (an equal-GAIN
// pair: the two taps are the same signal a grain apart, so they add
// coherently), and each tap's jump back to delay W lands at its window's zero.
class OctaveUpShifter
{
public:
    static constexpr float kGrainMs = 40.0f;

    void prepare(double sampleRate)
    {
        grain = juce::jmax(16, static_cast<int>(std::round(kGrainMs * 0.001 * sampleRate)));
        int size = 1;
        while (size < grain + 4) size <<= 1;
        buffer.assign(static_cast<size_t>(size), 0.0f);
        mask = size - 1;
        reset();
    }

    void reset()
    {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
        phase = 0.0f;
    }

    float process(float x)
    {
        buffer[static_cast<size_t>(writePos)] = x;
        float out = 0.0f;
        for (int tap = 0; tap < 2; ++tap) {
            float p = phase + 0.5f * static_cast<float>(tap);
            if (p >= 1.0f) p -= 1.0f;
            const float delay = static_cast<float>(grain) * (1.0f - p);   // W -> 0
            const float w = std::sin(juce::MathConstants<float>::pi * p);
            out += w * w * read(delay);
        }
        phase += 1.0f / static_cast<float>(grain);
        if (phase >= 1.0f) phase -= 1.0f;
        writePos = (writePos + 1) & mask;
        return out;
    }

private:
    float read(float delay) const
    {
        const float pos = static_cast<float>(writePos) - delay;
        const float fl = std::floor(pos);
        const float frac = pos - fl;
        const int i0 = static_cast<int>(fl) & mask;
        const int i1 = (i0 + 1) & mask;
        return buffer[static_cast<size_t>(i0)] + frac * (buffer[static_cast<size_t>(i1)] - buffer[static_cast<size_t>(i0)]);
    }

    std::vector<float> buffer;
    int mask = 0;
    int writePos = 0;
    int grain = 1764;
    float phase = 0.0f;
};
