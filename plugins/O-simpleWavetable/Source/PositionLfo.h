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

    O-simpleWavetable - global position LFO (Stage 2.3, RESEARCH 2.2)
    Ouaricon Audio
    Developer: Taylor Brook

    ONE global phase (double) for the whole instance: reset to 0 in prepare,
    never by notes, never on a sync change. Rendered per sample into a
    buffer preallocated in prepare.

      Free mode   : 0.01..20 Hz, phase accumulates per sample.
      Tempo mode  : host playing with valid PPQ + BPM -> the phase is computed
                    ABSOLUTELY per sample, frac((ppq0 + i * dppq) / beats).
                    It re-locks every block (loops, jumps), no drift.
                    Stopped / no playhead -> free-run at the host BPM, or 120.
      S&H (D-J)   : value = splitmix64 hash of (seed, cycleIndex). Position-
                    addressable: deterministic, block-size independent, and a
                    transport loop replays the same values. In free mode the
                    cycle index counts wraps since prepare.

    kDivBeats are EXACT doubles (4.0 / 3.0 ...), never rounded triplet
    literals (a 4-bar loop would drift).

    Triangle starts at 0 going up (ARCHITECTURE 9).

    Real-time: render() allocates nothing; prepare() (message thread) sizes
    the buffer.

  ==============================================================================
*/

#pragma once

#include "TestHooks.h"   // first: the OSIW_TEST_HOOKS default (Stage 2 note 8)

#include <juce_audio_basics/juce_audio_basics.h>

#include <cmath>
#include <cstdint>
#include <vector>

class PositionLfo
{
public:
    enum Shape { Sine = 0, Triangle, Saw, Square, SampleHold };

    static constexpr int kNumShapes    = 5;
    static constexpr int kNumDivisions = 16;

    // Beats per cycle, in lfo_div choice order (parameter-spec):
    // 4 bars, 2 bars, 1/1, 1/2, 1/4, 1/8, 1/16, 1/32,
    // 1/2., 1/4., 1/8., 1/16., 1/2T, 1/4T, 1/8T, 1/16T
    static constexpr double kDivBeats[kNumDivisions] = { 16.0, 8.0, 4.0, 2.0, 1.0, 0.5, 0.25, 0.125,
                                                         3.0, 1.5, 0.75, 0.375,
                                                         4.0 / 3.0, 2.0 / 3.0, 1.0 / 3.0, 1.0 / 6.0 };

    static constexpr std::uint64_t kDefaultSeed = 0x4F53695753482D31ull;   // constant: byte-stable renders

    // Seed for the S&H hash. Persists across prepare(). Recomputes the held
    // value for the current cycle so a seed change takes effect at once.
    void setSeed (std::uint64_t s) noexcept
    {
        seed = s;
        shValue = draw (cycleIndex);
    }

    void prepare (double sampleRate, int maxBlock)
    {
        fs = (sampleRate > 0.0 && std::isfinite (sampleRate)) ? sampleRate : 44100.0;
        buf.assign ((size_t) juce::jmax (1, maxBlock), 0.0f);
        phase = 0.0;
        cycleIndex = 0;
        shValue = draw (0);
       #if OSIW_TEST_HOOKS
        blockStartPhase = 0.0;
       #endif
    }

    struct Transport
    {
        bool   valid   = false;   // finite BPM > 0 reported
        bool   playing = false;   // playing AND valid PPQ AND valid BPM
        double ppq     = 0.0;
        double bpm     = 120.0;
    };

    // Audio thread. getIsPlaying() is a plain bool; getBpm / getPpqPosition
    // are Optional (JUCE 8.0.15).
    static Transport readTransport (juce::AudioPlayHead* ph) noexcept
    {
        Transport t;
        if (ph != nullptr)
        {
            if (const auto pos = ph->getPosition())
            {
                const auto bpm = pos->getBpm();
                if (bpm.hasValue() && std::isfinite (*bpm) && *bpm > 0.0)
                {
                    t.bpm = *bpm;
                    t.valid = true;
                }
                const auto ppq = pos->getPpqPosition();
                t.playing = pos->getIsPlaying() && ppq.hasValue() && t.valid && std::isfinite (*ppq);
                if (t.playing)
                    t.ppq = *ppq;
            }
        }
        return t;                                       // invalid -> 120 BPM free-run
    }

    // Transport for a sub-range starting 'offset' samples into the block
    // (oversized-host-block chunk loop): PPQ advanced by offset samples.
    Transport offsetBy (const Transport& t, int offset) const noexcept
    {
        Transport r = t;
        if (r.playing && offset > 0)
            r.ppq = t.ppq + (double) offset * t.bpm / (60.0 * fs);
        return r;
    }

    // numSamples <= the prepared capacity (the processor chunks at preparedBlock).
    const float* render (int numSamples, Shape shape, bool tempoMode, double rateHz, int divIndex,
                         const Transport& t) noexcept
    {
        float* out = buf.data();
        jassert (numSamples <= (int) buf.size());
        numSamples = juce::jlimit (0, (int) buf.size(), numSamples);

        if (tempoMode)
        {
            const double beats = kDivBeats[juce::jlimit (0, kNumDivisions - 1, divIndex)];
            const double bpm   = t.valid ? t.bpm : 120.0;

            if (t.playing)
            {
                const double dppq = bpm / (60.0 * fs);
                for (int i = 0; i < numSamples; ++i)
                {
                    const double cyc = (t.ppq + (double) i * dppq) / beats;   // ABSOLUTE: re-locks every block
                    const double fl  = std::floor (cyc);
                    advanceCycle ((std::int64_t) fl);
                    phase = cyc - fl;
                   #if OSIW_TEST_HOOKS
                    if (i == 0)
                        blockStartPhase = phase;
                   #endif
                    out[i] = shapeAt (shape, phase);
                }
                // Continue seamlessly from the next sample if the transport stops.
                if (numSamples > 0)
                {
                    phase += dppq / beats;
                    phase -= std::floor (phase);
                }
                return out;
            }

            rateHz = bpm / 60.0 / beats;                // stopped / Standalone: free-run
        }

       #if OSIW_TEST_HOOKS
        blockStartPhase = phase;
       #endif

        const double r   = std::isfinite (rateHz) ? rateHz : 0.0;
        const double inc = juce::jlimit (0.0, 0.5, r / fs);
        for (int i = 0; i < numSamples; ++i)
        {
            out[i] = shapeAt (shape, phase);
            phase += inc;
            if (phase >= 1.0)
            {
                phase -= std::floor (phase);
                advanceCycle (cycleIndex + 1);
            }
        }
        return out;
    }

    // Display atomic: the last rendered value of a render of n samples.
    float lastValue (int n) const noexcept
    {
        return (n > 0 && n <= (int) buf.size()) ? buf[(size_t) n - 1] : 0.0f;
    }

    const float* data() const noexcept { return buf.data(); }

   #if OSIW_TEST_HOOKS
    double getBlockStartPhase() const noexcept { return blockStartPhase; }   // phase of sample 0 of the last render
    double getPhase() const noexcept           { return phase; }             // phase of the NEXT sample
    std::int64_t getCycleIndex() const noexcept { return cycleIndex; }
   #endif

private:
    void advanceCycle (std::int64_t ci) noexcept
    {
        if (ci != cycleIndex)
        {
            cycleIndex = ci;
            shValue = draw (ci);
        }
    }

    // splitmix64 finaliser of (seed, cycleIndex) -> uniform [-1, 1).
    float draw (std::int64_t ci) const noexcept
    {
        std::uint64_t z = seed ^ ((std::uint64_t) ci * 0x9E3779B97F4A7C15ull);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        z ^= z >> 31;
        return (float) ((double) (z >> 40) * (1.0 / 16777216.0) * 2.0 - 1.0);
    }

    float shapeAt (Shape s, double p) const noexcept
    {
        switch (s)
        {
            case Sine:       return (float) std::sin (juce::MathConstants<double>::twoPi * p);
            case Triangle:   return (float) (p < 0.25 ? 4.0 * p : (p < 0.75 ? 2.0 - 4.0 * p : 4.0 * p - 4.0));
            case Saw:        return (float) (2.0 * p - 1.0);
            case Square:     return p < 0.5 ? 1.0f : -1.0f;
            case SampleHold: return shValue;
        }
        return 0.0f;
    }

    double fs = 44100.0;
    double phase = 0.0;
    std::int64_t cycleIndex = 0;
    float shValue = 0.0f;
    std::uint64_t seed = kDefaultSeed;
    std::vector<float> buf;

   #if OSIW_TEST_HOOKS
    double blockStartPhase = 0.0;
   #endif
};
